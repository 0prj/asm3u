__author__ = 'Li Dawen'

from array import array
import serial
import time
import sys
import packge

'''
#define PACKGE_ERR_NOT						'0'
#define PACKGE_ERR_PACKGE_TYPE_ERROR		'1'
#define PACKGE_ERR_PACKGE_DATA_TIME_OUT		'2'
#define PACKGE_ERR_PACKGE_DATA_CRC_ERROR	'3'
#define PACKGE_ERR_PACKGE_INDEX_ERROR		'4'
#define PACKGE_ERR_FLASH_ERROR				'5'
#define PACKGE_ERR_BIN_CRC_ERROR			'6'
#define PACKGE_ERR_PACKGE_INFO_VALUE_ERROR	'7'
#define PACKGE_ERR_PACKGE_DATA_LEN_ERROR	'8'
#define PACKGE_ERR_QUIT						'9'
'''

if __name__ == '__main__':
    print "file name", sys.argv[0]
    for i in range(1, len(sys.argv)):
        print 'var', i, sys.argv[i]
    packge = packge.PACKGE()

    print 'hello world'
    print sys.prefix
    packge_size = 1024
    if(sys.argv > 1):
        filename = sys.argv[1]
        file_bin = open(filename, 'rb')
        print("filename=", filename)
    data = file_bin.read()
    g_time = time.time()
    s=serial.Serial(port='COM6', baudrate=115200, timeout=0.01)

    def check_respons(respons, time_out):
        err_no = 99
        time_count = time_out
        while(time_count>0):
            time.sleep(0.001)
            in_count = s.inWaiting()
            time_count -= 1
            if(0 < in_count):
                respons=respons+s.read(in_count)
                if(respons.find('\n') > 0):
                    if (respons.find('ERR') >= 0):
                        p_sn = respons.find('ERR')
                        err_no =  ord(respons[respons.find('ERR')+3] )-ord('0')
                        break
                    elif(respons.find('END') >= 0):
                        err_no = 11
                        break
                    elif(respons.find('OK') >= 0):
                        err_no = 0
                        break
                    else:
                        err_no = 99
                        break
        print time.time(),'[',err_no,']:', respons
        return err_no, respons

    def send_h_packge(err_type = -1):
        head_buf = packge.build_head(0x80000000, 0x80008000, 256, 284, data)
        send_buf = packge.build_data(ord('H'),0,284,head_buf)
        if(err_type>=0):
            #ERR1
            send_buf[err_type]= chr(0xFF)
        elif(err_type == -2):
            #ERR2
            del(send_buf[-1])
            print '### send buf len:',len(send_buf),'err_type:',err_type
        elif(err_type == -11):
            #ERR2
            del(send_buf[2:-1])
            print '### send buf len:',len(send_buf),'err_type:',err_type
        elif(err_type == -6):
            #ERR6 :#define PACKGE_ERR_BIN_CRC_ERROR			'6'
            head_buf[2] = chr(0xFF)
            send_buf = packge.build_data(ord('H'),0,284,head_buf)
            print '### send buf len:',len(send_buf),'err_type:',err_type
        elif(err_type == -7):
            #ERR7:
            head_buf[6] = chr(0x55)
            send_buf = packge.build_data(ord('H'),0,284,head_buf)
            print '### send buf len:',len(send_buf),'err_type:',err_type
        elif(err_type == -100):
            #ERR7
            send_buf = packge.build_data(ord('H'),0,284,head_buf)
            for i in range(8000):
                send_buf.append(chr(0xFF))
            print '### send buf len:',len(send_buf),'err_type:',err_type
        print 'send H packge len:',len(send_buf)
        s.write(bytearray(send_buf))
        ch = ''
        rt_no=0
        rt_no, ch = check_respons(ch,3000)
        return rt_no, ch

    def send_d_packge(packge_size = 1024, err_index=0, err_type = 0):
        load_size = len(data)
        index = 1
        while(load_size ):
            in_data = ''
            if(load_size > packge_size):
                tran_size = packge_size
            else:
                tran_size = load_size

            if(err_index == index):
                index = index+1

            send_buf = packge.build_data(ord('D'),index, tran_size, data[packge_size*(index-1):index*packge_size])
            print 'send D packge index:',index,'len:',len(send_buf)
            index += 1
            load_size = load_size - tran_size
            s.write(bytearray(send_buf))

            ch = ''
            rt_no = 0
            rt_no, ch = check_respons(ch,3000)
            if(rt_no > 0):
                break
            if(load_size == 0):
                time.sleep(1)
                rt_no, ch = check_respons(ch,3000)

        return rt_no, ch
    def case_update_ok():
        #updata ok
        send_h_packge()
        send_d_packge()
        return
    '''
    #define PACKGE_ERR_NOT						'0'
    #define PACKGE_ERR_PACKGE_TYPE_ERROR		'1'
    #define PACKGE_ERR_PACKGE_DATA_TIME_OUT		'2'
    #define PACKGE_ERR_PACKGE_DATA_CRC_ERROR	'3'
    #define PACKGE_ERR_PACKGE_INDEX_ERROR		'4'
    #define PACKGE_ERR_FLASH_ERROR				'5'
    #define PACKGE_ERR_BIN_CRC_ERROR			'6'
    #define PACKGE_ERR_PACKGE_INFO_VALUE_ERROR	'7'
    #define PACKGE_ERR_PACKGE_DATA_LEN_ERROR	'8'
    #define PACKGE_ERR_QUIT						'9'

    '''
    def do_to_update_mode():
        flag,respons = do_port_check_input('U',600)
        if(flag):
            print 'is update mode:', respons
        else:
            s.write('A')
            flag,respons = do_port_check_input('U',600)
            if(flag):
                print 'to update mode:', respons
            else:
                print 'to update mode fail:', respons

    def do_to_command_mode():
        print 'to command mode'
        do_port_send_cmd('Q','ERR9',0)

    def do_show_trace_report():
        do_to_command_mode()
        do_port_send_cmd('r','',0)
        print 'trace report:'

    def time_start():
        g_time = time.time()

    def time_end():
        m_time = time.time()
        print 'RUN TIME:',m_time -g_time
        return m_time- g_time

    def do_port_check_input(exp_respons = '', time_out = 600):
        time_count = time_out
        respons = ''
        time_start()
        while(time_count>0):
            time.sleep(0.001)
            in_count = s.inWaiting()
            time_count -= 1
            if(0 < in_count):
                respons=respons+s.read(in_count)
                if(respons.find(exp_respons) >= 0):
                    print 'check respons:', respons
                    return True, respons
        print 'check respons:', respons
        time_end()
        return False, respons

    def do_port_send_cmd(cmd = '', exp_respons = '', time_out = 600):
        print 'send command:',cmd,'wait:',exp_respons
        s.write(cmd)
        return do_port_check_input(exp_respons,time_out)

    def do_case(case_name):
        rt_no = 0
        ch=''
        expected = case_name

        do_to_update_mode()

        if('OK'==case_name):
            rt_no, ch = send_h_packge()
            do_port_send_cmd('A','')
        elif('ERR1'==case_name):
            rt_no, ch = send_h_packge(0)
        elif('ERR2'==case_name):
            rt_no, ch = send_h_packge(-2)
        elif('ERR3'==case_name):
            rt_no, ch = send_h_packge(283)
        elif('ERR4'==case_name):
            rt_no, ch = send_h_packge()
            time.sleep(3)
            rt_no, ch = send_d_packge(1024,1,1)
        elif('ERR5'==case_name):
            #lock flash @80008000
            do_to_command_mode()
            do_port_send_cmd('v','ERR5')
            do_to_update_mode()
            rt_no, ch = send_h_packge()
            rt_no, ch = send_d_packge()
            do_port_send_cmd('x','OK')
            #unlock flash @80008000
        elif('ERR6'==case_name):
            rt_no, ch = send_h_packge(-6)
            if(0 == rt_no):
                rt_no, ch = send_d_packge()
        elif('ERR7'==case_name):
            rt_no, ch = send_h_packge(-7)
        elif('ERR8'==case_name):
            rt_no, ch = send_h_packge()
            if(0 == rt_no):
                rt_no, ch = send_d_packge(4097,0,0)
        elif('ERR9'==case_name):
            s.write('Q')
            rt_no, ch = check_respons(ch,3000)
        elif('END'==case_name):
            rt_no, ch = send_h_packge()
            if(0 == rt_no):
                rt_no, ch = send_d_packge()
        elif('ERR1A'==case_name):
            expected = 'ERR1'
            rt_no, ch = send_h_packge(-11)
        elif('OKA'==case_name):
            expected = 'OK'
            rt_no, ch = send_h_packge(-100)
        elif('ERR9A'==case_name):
            expected = 'ERR9'
            rt_no, ch = send_h_packge()
            ch = ''
            s.write('Q')
            rt_no, ch = check_respons(ch,3000)
        elif('ENDA'==case_name):
            expected = 'END'
            rt_no, ch = send_h_packge()
            print 'delay 6s'
            time.sleep(6)
            if(0 == rt_no):
                rt_no, ch = send_d_packge()
        time.sleep(0.1)

        if(ch.find(expected) >= 0):
            print '!!! CASE PASS !!! name:',case_name,'expected:',expected,'respons',ch
            return True, ch
        else:

            print '### CASE FAIL ### name:',case_name,'expected:',expected,'respons',ch
            return False,ch

    def case_cover():
        case_names = ['OK','OKA','ERR1','ERR1A','ERR2','ERR3','ERR4','ERR5','ERR6','ERR7','ERR8','ERR9','ERR9A','END','ENDA']
        case_is_pass = False
        ch = ''
        for name in case_names:
            case_is_pass,ch = do_case(name)
            if(False == case_is_pass):
                print 'CASE:',name,'is FAIL'
                break
        case_is_pass,ch = do_port_send_cmd('r', 'OK')
        if(ch.find('PASS')>=0):
            print 'COVER TEST IS PASS !!!'
        else:
            print 'COVER TEST IS FAIL !!!'
        return

    while(1):
        key = raw_input("boot>")
        if(key.find('p')>=0):
            print "continue"
            continue
        elif(key.find('h')>=0):
            print "run h"
            send_h_packge()
            continue
        elif(key.find('d')==0):
            print "run d"
            rt_no,ch = send_d_packge()
            if(0 == rt_no):
                print 'go to FW run'
                s.write('G')
            continue
        elif ((key.find('ERR')>=0) | (key.find('OK')>=0) |(key.find('END')>=0)):
            do_case(key)
            continue
        elif(key.find('m')>=0):
            do_to_update_mode()
            continue
        elif(key.find('c')>=0):
            case_cover()
            continue
        elif(key.find('q')>=0):
            print "run q"
            break
        print "write(", len(key),"):",key
        s.write(key)
        time.sleep(0.1)
        in_count = s.inWaiting()
        if(0 < in_count):
            ch=s.read(in_count)
            print in_count,':', ch,  time.time()
    s.close()
