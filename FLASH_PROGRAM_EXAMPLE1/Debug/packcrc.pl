#!/usr/bin/perl

#THOR 0x54484F52 
# add head info to bin file   includes the bin by CRC sum

$fileName=$ARGV[0];
$fileName2=$ARGV[1];

# struct IMAGE_HEAD_INFO {
	# unsigned short magic;
	# unsigned short head_crc;
	# unsigned short data_type;    
	# unsigned short data_crc;
	# unsigned long data_size;
	# unsigned long data_load_addr;
# }IMAGE_HEAD_INFO_T;

open(FIN, '<', "$fileName")or die "Could not open the bin file $fileName:$!\n";
undef $/;
open(FOUT, '>',"$fileName2") or die "could not open the new file: $!\n";
binmode(FIN);
binmode(FOUT);

$str_data=<FIN>; 
$len_data=length($str_data);
$len_data_word = $len_data/4;

print("\n$fileName size:$len_data\n");
(@ary_data)=unpack("L$len_data_word", $str_data);
(@ary_data_byte)=unpack("C$len_data", $str_data);
$data_crc = crc16_buf(0,0x1021,$len_data, @ary_data_byte);

@ary_head[1] = 0x0B|$data_crc<<16;	#data_crc
@ary_head[2] = $len_data;				#data size
@ary_head[3] = 0x8000;				#data load addr

for($i=0; $i<3; $i=$i+1){
	@ary_head_byte[0+$i*4]=(@ary_head[$i+1]>>0)&0xFF;
	@ary_head_byte[1+$i*4]=(@ary_head[$i+1]>>8)&0xFF;
	@ary_head_byte[2+$i*4]=(@ary_head[$i+1]>>16)&0xFF;
	@ary_head_byte[3+$i*4]=(@ary_head[$i+1]>>24)&0xFF;
}
$head_crc = crc16_buf(0,0x1021, 12, @ary_head_byte);
@ary_head[0] = 0x5448|$head_crc<<16;     	#magic

print("\nHEAD INFO:\n");
for($i=0; $i<4; $i=$i+1){
	syswrite(FOUT, pack("L",@ary_head[$i]), 4);
	printf("%08X\n",@ary_head[$i]);
}

for($i=0; $i<($len_data_word); $i=$i+1){
	syswrite(FOUT, pack("L",@ary_data[$i]), 4);
}

$len_data +=16;
print("\n$fileName2 size:$len_data\n");
print("\ndone.\n");
close(FIN);
close(FOUT);

sub crc16_buf
{
	my ($init, $poly, $len_byte, @ary) = @_;
	for($i=0; $i<$len_byte; $i=$i+1)
	{
		$init = crc16($init,$poly,@ary[$i]);
	}
	return $init;
}

sub crc16
{
	my ($init, $poly, $val) = @_;
	die "Undefined CRC value" 
	unless defined $val && defined $poly && defined $init;
	my $var;
	for (my $i=0; $i<8; $i++)
	{
		$var = $val << 8;
		$var ^= $init;
		$init += $init;
		$val <<= 1;
		if ($var & 0x8000)
		{
			$init ^= $poly;
		}
	}
	return $init & 0xFFFF;
}
