研究Xmodem协议必看的11个问题
http://blog.sina.com.cn/s/blog_4db10c6c0100av57.html
Xmodem协议作为串口数据传输主要的方式之一，恐怕只有做过bootloader的才有机会 接触一下，网上有关该协议的内容要么是英语要么讲解不详细。笔者以前写bootloader时研究过1k-Xmodem，参考了不少相关资料。这里和大家交流一下我对Xmodem的理解，多多指教！

1．Xmodem协议是什么？
　　XMODEM协议是一种串口通信中广泛用到的异步文件传输协议。分为标准Xmodem和1k-Xmodem两种，前者以128字节块的形式传输数据，后者字节块为1k即1024字节，并且每个块都使用一个校验和过程来进行错误检测。在校验过程中如果接收方关于一个块的校验和与它在发送方的校验和相同时，接收方就向发送方发送一个确认字节(ACK)。由于Xmodem需要对每个块都进行认可，这将导致性能有所下降，特别是延时比较长的场合，这种协议显得效率更低。
    除了Xmodem，还有Ymodem，Zmodem协议。他们的协议内容和Xmodem类似，不同的是Ymodem允许批处理文件传输，效率更高；Zmodem则是改进的了Xmodem，它只需要对损坏的块进行重发，其它正确的块不需要发送确认字节。减少了通信量。
2．Xmodem协议相关控制字符
    SOH             0x01
    STX           0x02
    EOT             0x04
    ACK             0x06
    NAK             0x15
    CAN             0x18
    CTRLZ         0x1A
3．标准Xmodem协议（每个数据包含有128字节数据）帧格式
 _______________________________________________________________
|     |            |                   |          |            |
| SOH | 信息包序号 | 信息包序号的补码  | 数据区段 |  校验和    |
|_____|____________|___________________|__________|____________|
4．1k-Xmodem（每个数据包含有1024字节数据）帧格式
 _______________________________________________________________
|     |            |                   |          |            |
| STX | 信息包序号 |  信息包序号的补码 | 数据区段 |  校验和    |
|_____|____________|___________________|__________|____________|
5．数据包说明
    对于标准Xmodem协议来说，如果传送的文件不是128的整数倍，那么最后一个数据包的有效内容肯定小于帧长，不足的部分需要用CTRL-Z(0x1A)来填充。这里可能有人会问，如果我传送的是bootloader工程生成的.bin文件，mcu收到后遇到0x1A字符会怎么处理？其实如果传送的是文本文件，那么接收方对于接收的内容是很容易识别的，因为CTRL-Z不是前128个ascii码，不是通用可见字符，如果是二进制文件，mcu其实也不会把它当作代码来执行。哪怕是excel文件等，由于其内部会有些结构表示各个字段长度等，所以不会读取多余的填充字符。否则Xmodem太弱了。对于1k-Xmodem，同上理。
6．如何启动传输？
    传输由接收方启动，方法是向发送方发送"C"或者NAK(注意哦，这里提到的NAK是用来启动传输的。以下我们会看到NAK还可以用来对数据产生重传的机制)。接收方发送NAK信号表示接收方打算用累加和校验；发送字符"C"则表示接收方想打算使用CRC校验（具体校验规则下文Xmodem源码，源码胜于雄辩)。
7．传输过程
    当接收方发送的第一个"C"或者NAK到达发送方，发送方认为可以发送第一个数据包，传输已经启动。发送方接着应该将数据以每次128字节的数据加上包头，包号，包号补码，末尾加上校验和，打包成帧格式传送。
发送方发了第一包后就等待接收方的确认字节ACK，收到接收方传来的ACK确认，就认为数据包被接收方正确接收，并且接收方要求发送方继续发送下一个包；如果发送方收到接收方传来的NAK(这里，NAK用来告诉发送方重传，不是用来启动传输)字节，则表示接收方请求重发刚才的数据包；如果发送方收到接收方传来的CAN字节，则表示接收方请求无条件停止传输。
8．如何结束传输？
    如果发送方正常传输完全部数据，需要结束传输，正常结束需要发送方发送EOT 字节通知接收方。接收方回以ACK进行确认。当然接收方也可强制停止传输，当接收方发送CAN 字节给发送方，表示接收方想无条件停止传输，发送方收到CAN后，不需要再发送 EOT确认（因为接收方已经不想理它了，呵呵）。
9．特殊处理
    虽然数据包是以 SOH 来标志一个信息包的起始的，但在 SOH 位置上如果出现EOT则表示数据传输结束，再也没有数据传过来。
接收方首先应确认数据包序号的完整性，通过对数据包序号取补，然后和数据包序号的补码异或，结果为0表示正确，结果不为0则发送NAK请求重传。
    接收方确认数据包序号正确后，然后检查是否期望的序号。如果不是期望得到的数据包序号，说明发生严重错误，应该发送一个 CAN 来中止传输。
    如果接收到的数据包的包序号和前一包相同，那么接收方会忽略这个重复包，向发送方发出 ACK ，准备接收下一个包。
    接收方确认了信息包序号的完整性和是正确期望的后，只对 128 字节的数据区段进行算术和校验，结果与帧中最后一个字节（算术校验和）比较，相同发送 ACK，不同发送 NAK。
10．校验和的说明
    Xmodem协议支持2种校验和，它们是累加和与CRC校验。
    当接收方一开始启动传输时发送的是NAK，表示它希望以累加和方式校验。
    当接收方一开始启动传输时发送的是字符“C”，表示它希望以CRC方式校验。
    可能有人会问，接收方想怎么校验发送方都得配合吗，难道发送方必须都支持累加和校验和CRC校验？事实上Xmodem要求支持CRC的就必须同时支持累加和，如果发送方只支持累加和，而接收方用字符“C”来启动，那么发送方只要不管它，当接收方继续发送“C”，三次后都没收到应答，就自动会改为发送NAK，因为它已经明白发送方可能不支持CRC校验，现在接收方改为累加和校验和发送方通讯。发送方收到NAK就赶紧发送数据包响应。
11．Xmodem协议代码
        看了以上说明，再参考代码，应该很容易会理解代码编写者的思路。
