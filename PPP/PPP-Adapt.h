#define TCP_FLAG_ACK (1<<4)
#define TCP_FLAG_SYN (1<<1)
#define TCP_FLAG_PSH (1<<3)
#define TCP_FLAG_RST (1<<2)
#define TCP_FLAG_FIN (1<<0)

/// PPP header
	typedef struct { // [ff 03 00 21]
		unsigned int address : 8;  // always 0xff
		unsigned int control : 8;  // always 03
		unsigned int protocolR : 16; // byte reversed, 0x0021 for ip
	} pppHeader;

	/// LCP and IPCP header
	typedef struct {
		// ppp part
		unsigned int address : 8;  // always 0xff
		unsigned int control : 8;  // always 03
		unsigned int protocolR : 16; // byte reversed, 0x0021 for ip

									 // ipcp and lcp part
		unsigned int code : 8; // IPCP and LCP contain a code field which identifies the requested action or response
		unsigned int identifier : 8;
		unsigned int lengthR : 16;
		char * request;
	} ipcpHeader;

	/// IP header
	typedef struct {
		unsigned int headerLength : 4;  // ip headerlength / 4
		unsigned int version : 4;  // ip version number
		unsigned int ect : 1;  // ecn capable transport
		unsigned int ce : 1;  // ecn-ce
		unsigned int dscp : 6;  // differentiated services
		unsigned int lengthR : 16;  // ip packet length (byte-reversed)

		unsigned int identR : 16;  // ident, byte reversed
		unsigned int fragmentOffsHi : 5;
		unsigned int lastFragment : 1;
		unsigned int dontFragment : 1;
		unsigned int reservedIP : 1;
		unsigned int fragmentOffsLo : 8;

		unsigned int ttl : 8;
		unsigned int protocol : 8;  // next protocol
		unsigned int checksumR : 16;  // ip checksum, byte reversed
		union {
			unsigned int srcAdrR; // source IP address
			char * srcAdrPtr; // so we also have a char * to srcAdrR
		};
		union {
			unsigned int dstAdrR; // destination IP address
			char * dstAdrPtr;  // so we also have a char * to dstAdrR
		};
	} ipHeaderType;

	/// IP pseudoheader. Used in TCP/UDP checksum calculations.
	typedef struct {
		union {
			char * start; // a char * to avoid type conversions
			unsigned int srcAdrR; // source IP address
		};
		unsigned int dstAdrR; // destination IP address
		unsigned int zero : 8;
		unsigned int protocol : 8;
		unsigned int lengthR : 16; // byte reversed
	} pseudoIpHeaderType;

	/// TCP header
	typedef struct {
		unsigned int    srcPortR : 16; // byte reversed
		unsigned int    dstPortR : 16; // byte reversed
		unsigned int    seqTcpR;      // byte reversed
		unsigned int    ackTcpR;      // byte reversed
		unsigned int resvd1 : 4;  // reserved
		unsigned int offset : 4; // tcp header length [5..15]
		union {
			unsigned char All;  // all 8 flag bits
			struct {            // individual flag bits
				unsigned char fin : 1, // fin
					syn : 1, // syn
					rst : 1, // rst
					psh : 1, // psh
					ack : 1, // ack
					urg : 1, // urg
					ece : 1, // ece
					cwr : 1; // cwr
			};
		} flag;
		unsigned int windowR : 16; // byte reversed
		unsigned int    checksumR : 16; // byte reversed
		unsigned int    urgentPointerR : 16; // byte reversed;
		unsigned int    tcpOptions[10]; // up to 10 words of options possible
	} tcpHeaderType;

	/// UDP header.
	typedef struct {
		unsigned int srcPortR : 16; // byte reversed
		unsigned int dstPortR : 16; // byte reversed
		unsigned int lengthR : 16; // byte reversed
		unsigned int checksumR : 16; // byte reversed
		char * data; // data area
	} udpHeaderType;

	/// ICMP header.
	typedef struct {
		unsigned int type : 8;
		unsigned int code : 8;
		unsigned int checkSumR : 16; // byte reversed
		unsigned int idR : 16; // byte reversed
		unsigned int sequenceR : 16; // byte reversed
		char * data; // data area
	} icmpHeaderType;

	/// Structure to manage all ppp variables.
	typedef struct pppType {
		union {
			pppHeader * ppp; // pointer to ppp structure
			ipcpHeader * ipcp; // pointer to ipcp structure
			ipcpHeader * lcp; // pointer to lcp structure (same as ipcp)
		};
		union {
			ipHeaderType * ip; // pointer to ip header struct
			char * ipStart; // char pointer to ip header struct (need a char pointer for byte offset calculations)
		};
		union { // a union for the packet type contained in the IP packet
			tcpHeaderType  * tcp;  // pointer to tcp header struct
			udpHeaderType  * udp;  // pointer to udp header struct
			icmpHeaderType * icmp; // pointer to udp header struct
			char * tcpStart;  // char pointer to tcp header struct  (need a char pointer for byte offset calculations)
			char * udpStart;  // char pointer to udp header struct  (need a char pointer for byte offset calculations)
			char * icmpStart; // char pointer to icmp header struct (need a char pointer for byte offset calculations)
		};
		char * tcpData; // char pointer to where tcp data starts
		int online; // we hunt for a PPP connection if this is zero
		int hostIP; // ip address of host
		int fcs; // PPP "frame check sequence" - a 16-bit HDLC-like checksum used in all PPP frames
		int ledState; // state of LED1
		int responseCounter;
		int firstFrame; // cleared after first frame
		unsigned int sum; // a checksum used in headers
		struct {
#define RXBUFLEN (1<<11)
			// the serial port receive buffer and packet buffer, size is RXBUFLEN (currently 2048 bytes)
			char buf[RXBUFLEN]; // RXBUFLEN MUST be a power of two because we use & operator for fast wrap-around in ring buffer
			volatile int head; // declared volatile so user code knows this variable changes in the interrupt handler
			int tail;
			int rtail;
			int buflevel;
		} rx; // serial port objects
		struct {
			int len; // number of bytes in buffer
			int crc; // PPP CRC (frame check)
#define PPP_max_size 1600
					 // we are assuming 100 bytes more than MTU size of 1500
			char buf[PPP_max_size]; // send and receive buffer large enough for largest IP packet
		} pkt; // ppp buffer objects
		struct {
			int frameStartIndex; // frame start marker
			int frameEndIndex; // frame end marker
		} hdlc; // hdlc frame objects
		struct {
			unsigned int ident; // our IP ident value (outgoing frame count)
		} ipData; // ip related object
	} pppVariables;

	pppType ppp;

	/// Initialize the ppp structure and clear the receive buffer
	void pppInitStruct()
	{
		memset(ppp.rx.buf, 0, RXBUFLEN);
		ppp.online = 0;
		ppp.rx.tail = 0;
		ppp.rx.rtail = 0;
		ppp.rx.head = 0;
		ppp.rx.buflevel = 0;
		ppp.pkt.len = 0;
		ppp.ipData.ident = 10000; // easy to recognize in ip packet dumps
		ppp.ledState = 0;
		ppp.hdlc.frameStartIndex = 0;
		ppp.responseCounter = 0;
		ppp.firstFrame = 1;
		ppp.ppp = (pppHeader *)ppp.pkt.buf; // pointer to ppp header
		ppp.ip = (ipHeaderType *)(ppp.pkt.buf + 4); // pointer to IP header
	}

	/// PPP serial port receive interrupt handler.
	/// Check for available characters from the PC and read them into our own circular serial receive buffer at ppp.rx.buf.
	/// Also, if we are offline and a 0x7e frame start character is seen, we go online immediately
	void pppReceiveHandler()
	{
		char ch;
		while (serialport->isAvailable()) {
			int hd = (ppp.rx.head + 1)&(RXBUFLEN - 1); // increment/wrap head index
			if (hd == ppp.rx.rtail) {
				return;
			}
			ch = serialport->readByte(); // read new character
			ppp.rx.buf[ppp.rx.head] = ch; // insert in our receive buffer
			if (ppp.online == 0) {
				if (ch == 0x7E) {
					ppp.online = 1;
				}
			}
			ppp.rx.head = hd; // update head pointer
			ppp.rx.buflevel++;
		}
	}

	/// Initialize the PPP FCS (frame check sequence) total
	void fcsReset()
	{
		ppp.fcs = 0xffff;   // crc restart
	}

	/// update the cumulative PPP FCS (frame check sequence)
	void fcsDo(int x)
	{
		for (int i = 0; i<8; i++) {
			ppp.fcs = ((ppp.fcs & 1) ^ (x & 1)) ? (ppp.fcs >> 1) ^ 0x8408 : ppp.fcs >> 1; // crc calculator
			x >>= 1;
		}
		// handle input
	}

	/// calculate the PPP FCS (frame check sequence) on an entire block of memory
	int fcsBuf(char * buf, int size) // crc on an entire block of memory
	{
		fcsReset();
		for (int i = 0; i<size; i++)fcsDo(*buf++);
		return ppp.fcs;
	}

	/// Get one character from our received PPP buffer
	int pc_getBuf()
	{
		int x = ppp.rx.buf[ppp.rx.tail];
		ppp.rx.tail = (ppp.rx.tail + 1)&(RXBUFLEN - 1);
		ppp.rx.buflevel--;
		return x;
	}

	/// Process a received PPP frame
	void processPPPFrame(int start, int end)
	{
		if (start == end) {
			return; // empty frame
		}
		fcsReset();
		char * dest = ppp.pkt.buf;
		ppp.pkt.len = 0;
		int unstuff = 0;
		int idx = start;
		while (1) {

			if (unstuff == 0) {
				if (ppp.rx.buf[idx] == 0x7d) unstuff = 1;
				else {
					*dest = ppp.rx.buf[idx];
					ppp.pkt.len++;
					dest++;
					fcsDo(ppp.rx.buf[idx]);
				}
			}
			else { // unstuff characters prefixed with 0x7d
				*dest = ppp.rx.buf[idx] ^ 0x20;
				ppp.pkt.len++;
				dest++;
				fcsDo(ppp.rx.buf[idx] ^ 0x20);
				unstuff = 0;
			}
			idx = (idx + 1) & (RXBUFLEN - 1);
			if (idx == end) break;
		}
		ppp.pkt.crc = ppp.fcs & 0xffff;
		if (ppp.pkt.crc == 0xf0b8) { // check for good CRC
			determinePacketType();
		}
	}

	/// do PPP HDLC-like handling of special (flag) characters
	void hdlcPut(int ch)
	{
		if ((ch<0x20) || (ch == 0x7d) || (ch == 0x7e)) {
			serialport->writeByte(0x7d);
			serialport->writeByte(ch ^ 0x20);  // these characters need special handling
		}
		else {
			serialport->writeByte(ch);
		}
	}

	/// send a PPP frame in HDLC format
	void sendPppFrame()
	{
		ppp.responseCounter++; // count the number of ppp frames we send
		int crc = fcsBuf(ppp.pkt.buf, ppp.pkt.len - 2); // update crc
		ppp.pkt.buf[ppp.pkt.len - 2] = (~crc >> 0); // fcs lo (crc)
		ppp.pkt.buf[ppp.pkt.len - 1] = (~crc >> 8); // fcs hi (crc)
		serialport->writeByte(0x7e); // hdlc start-of-frame "flag"
		for (int i = 0; i<ppp.pkt.len; i++) {
			//wait_us(86); // wait one character time

			hdlcPut(ppp.pkt.buf[i]); // send a character
		}
		serialport->writeByte(0x7e); // hdlc end-of-frame "flag"
	}

	/// convert a network ip address in the buffer to an integer (IP adresses are big-endian, i.e most significant byte first)
	int bufferToIP(char * buffer)
	{
		int result = 0;
		for (int i = 0; i<4; i++) result = (result << 8) | (*buffer++ & 0xff);
		return result;
	}

	/// convert 4-byte ip address to 32-bit
	unsigned int ip(int a, int b, int c, int d)
	{
		return a << 24 | b << 16 | c << 8 | d;
	}

	/// handle IPCP configuration requests
	void ipcpConfigRequestHandler()
	{
		if (ppp.ipcp->request[0] == 3) {
			ppp.hostIP = bufferToIP(ppp.pkt.buf + 10);
			//debugPrintf("Host IP = %d.%d.%d.%d (%08x)\n", ppp.ipcp->request[2], ppp.ipcp->request[3], ppp.ipcp->request[4], ppp.ipcp->request[5], ppp.hostIP);
		}

		ppp.ipcp->code = 2; // change code to ack
		sendPppFrame(); // acknowledge everything they ask for - assume it's IP addresses

	//	debugPrintf("Our IPCP Ask (no options)\n");
		ppp.ipcp->code = 1; // change code to request
		ppp.ipcp->lengthR = htonl(4); // 4 is minimum length - no options in this request
		ppp.pkt.len = 4 + 4 + 2; // no options in this request shortest ipcp packet possible (4 ppp + 4 ipcp + 2 crc)
		sendPppFrame(); // send our request
	}

	/// handle IPCP acknowledge (do nothing)
	void ipcpAckHandler()
	{
		//debugPrintf("Their IPCP Grant\n");
	}

	/// Handle IPCP NACK by sending our suggested IP address if there is an IP involved.
	/// This is how Linux responds to an IPCP request with no options - Windows assumes any IP address on the submnet is OK.
	void ipcpNackHandler()
	{
		//debugPrintf("Their IPCP Nack\n");
		if (ppp.ipcp->request[0] == 3) { // check if the NACK contains an IP address parameter
			ppp.ipcp->code = 1; // assume the NACK contains our "suggested" IP address
			sendPppFrame(); // let's request this IP address as ours
			//debugPrintf("Our IPCP ACK (received an IP)\n");
		}
		else { // if it's not an IP nack we ignore it
			//debugPrintf("IPCP Nack Ignored\n");
		}
	}

	/// handle all other IPCP requests (by ignoring them)
	void ipcpDefaultHandler()
	{
		//debugPrintf("Their IPCP Other\n");
	}

	/// process an incoming IPCP packet
	void IPCPframe()
	{
		int action = ppp.ipcp->code; // packet type is here
		switch (action) {
		case 1:
			ipcpConfigRequestHandler();
			break;
		case 2:
			ipcpAckHandler();
			break;
		case 3:
			ipcpNackHandler();
			break;
		default:
			ipcpDefaultHandler();
		}
	}

	/// perform a 16-bit checksum. if the byte count is odd, stuff in an extra zero byte.
	unsigned int dataCheckSum(char * ptr, int len, int restart)
	{
		unsigned int i, hi, lo;
		unsigned char placeHolder;
		if (restart) ppp.sum = 0;
		if (len & 1) {
			placeHolder = ptr[len];
			ptr[len] = 0;  // if the byte count is odd, insert one extra zero byte is after the last real byte because we sum byte PAIRS
		}
		i = 0;
		while (i<len) {

			hi = ptr[i++];
			lo = ptr[i++];
			ppp.sum = ppp.sum + ((hi << 8) | lo);
		}
		if (len & 1) {
			ptr[len] = placeHolder;    // restore the extra byte we made zero
		}
		ppp.sum = (ppp.sum & 0xffff) + (ppp.sum >> 16);
		ppp.sum = (ppp.sum & 0xffff) + (ppp.sum >> 16); // sum one more time to catch any carry from the carry
		return ~ppp.sum;
	}

	/// perform the checksum on an IP header
	void IpHeaderCheckSum()
	{
		ppp.ip->checksumR = 0; // zero the checsum in the IP header
		int len = 4 * ppp.ip->headerLength; // length of IP header in bytes
		unsigned int sum = dataCheckSum(ppp.ipStart, len, 1);
		ppp.ip->checksumR = ntohs(sum); // insert fresh checksum
	}

	/// swap the IP source and destination addresses
	void swapIpAddresses()
	{
		unsigned int tempHold;
		tempHold = ppp.ip->srcAdrR; // tempHold <- source IP
		ppp.ip->srcAdrR = ppp.ip->dstAdrR; // source <- dest
		ppp.ip->dstAdrR = tempHold; // dest <- tempHold*/
	}

	/// swap the IP source and destination ports
	void swapIpPorts()
	{
		int headerSizeIP = 4 * (ppp.ip->headerLength); // calculate size of IP header
		char * ipSrcPort = ppp.ipStart + headerSizeIP + 0; // ip source port location
		char * ipDstPort = ppp.ipStart + headerSizeIP + 2; // ip destin port location
		char tempHold[2];
		memcpy(tempHold, ipSrcPort, 2); // tempHold <- source
		memcpy(ipSrcPort, ipDstPort, 2); // source <- dest
		memcpy(ipDstPort, tempHold, 2); // dest <- tempHold
	}

	/// Build the "pseudo header" required for UDP and TCP, then calculate its checksum
	void checkSumPseudoHeader(unsigned int packetLength)
	{
		// this header  contains the most important parts of the IP header, i.e. source and destination address, protocol number and data length.
		pseudoIpHeaderType pseudoHeader; // create pseudo header
		pseudoHeader.srcAdrR = ppp.ip->srcAdrR; // copy in ip source address
		pseudoHeader.dstAdrR = ppp.ip->dstAdrR; // copy in ip dest address
		pseudoHeader.zero = 0; // zero byte
		pseudoHeader.protocol = ppp.ip->protocol; // protocol number (udp or tcp)
		pseudoHeader.lengthR = htons(packetLength); // size of tcp or udp packet
		dataCheckSum(pseudoHeader.start, 12, 1); // calculate this header's checksum
	}

	/// initialize an IP packet to send
	void initIP(unsigned int srcIp, unsigned int dstIp, unsigned int srcPort, unsigned int dstPort, unsigned int protocol)
	{
		ppp.ppp->address = 0xff;
		ppp.ppp->control = 3;
		ppp.ppp->protocolR = htons(0x0021);
		ppp.ip->version = 4;
		ppp.ip->headerLength = 5; // 5 words = 20 bytes
		ppp.ip->identR = htons(ppp.ipData.ident++); // insert our ident
		ppp.ip->dontFragment = 1;
		ppp.ip->ttl = 128;
		ppp.ip->protocol = protocol; // udp
		ppp.ip->srcAdrR = htonl(srcIp);
		ppp.ip->dstAdrR = htonl(dstIp);
		ppp.udpStart = ppp.ipStart + 20; // calculate start of udp header
		ppp.udp->srcPortR = htons(srcPort); // source port
		ppp.udp->dstPortR = htons(dstPort); // dest port
	}


	/// Build a UDP packet from scratch
	void sendUdp(unsigned int srcIp, unsigned int dstIp, unsigned int srcPort, unsigned int dstPort, char * message, int msgLen)
	{
		struct {
			unsigned int ipAll; // length of entire ip packet
			unsigned int ipHeader; // length of ip header
			unsigned int udpAll; // length of entire udp packet
			unsigned int udpData; // length of udp data segment
		} len;
		len.ipHeader = 20; // ip header length
		len.udpData = msgLen; // udp data size
		len.udpAll = len.udpData + 8; // update local udp packet length
		len.ipAll = len.ipHeader + len.udpAll; // update IP Length
		initIP(srcIp, dstIp, srcPort, dstPort, 17); // init a UDP packet
		ppp.ip->lengthR = htons(len.ipAll); // update IP length in buffer
		ppp.udpStart = ppp.ipStart + len.ipHeader; // calculate start of udp header
		memcpy(ppp.udp->data, message, len.udpData); // copy the message to the buffer
		ppp.udp->lengthR = htons(len.udpAll); // update UDP length in buffer
		ppp.pkt.len = len.ipAll + 2 + 4; // update ppp packet length
		IpHeaderCheckSum();  // refresh IP header checksum
		checkSumPseudoHeader(len.udpAll); // get the UDP pseudo-header checksum
		ppp.udp->checksumR = 0; // before TCP checksum calculations the checksum bytes must be set cleared
		unsigned int pseudoHeaderSum = dataCheckSum(ppp.udpStart, len.udpAll, 0); // continue the TCP checksum on the whole TCP packet
		ppp.udp->checksumR = htons(pseudoHeaderSum); // tcp checksum done, store it in the TCP header
		sendPppFrame(); // send the UDP message back
	}

	/// Process an incoming UDP packet.
	/// If the packet starts with the string "echo " or "test" we echo back a special packet
	void UDPpacket()
	{
		struct {
			unsigned int all; // length of entire ip packet
			unsigned int header; // length of ip header
		} ipLength;

		struct {
			unsigned int all; // length of entire udp packet
			unsigned int data; // length of udp data segment
		} udpLength;

		ipLength.header = 4 * ppp.ip->headerLength; // length of ip header
		ppp.udpStart = ppp.ipStart + ipLength.header; // calculate start of udp header
		udpLength.all = ntohs(ppp.udp->lengthR); // size of udp packet
		udpLength.data = udpLength.all - 8; // size of udp data

		int echoFound = !strncmp(ppp.udp->data, "echo ", 5); // true if UDP message starts with "echo "
		int testFound = !strncmp(ppp.udp->data, "test", 4); // true if UDP message starts with "test"
		if ((echoFound) || (testFound)) { // if the UDP message starts with "echo " or "test" we answer back
			if (echoFound) {
				swapIpAddresses(); // swap IP source and destination
				swapIpPorts(); // swap IP source and destination ports
				memcpy(ppp.udp->data, "Got{", 4); // in the UDP data modify "echo" to "Got:"
				int n = 0;
				n = n + sprintf(n + ppp.udp->data + udpLength.data, "} UDP Server: PPP-Blinky\n"); // an appendix
				udpLength.data = udpLength.data + n; // update udp data size with the size of the appendix
													 // we may have changed data length, update all the lengths
				udpLength.all = udpLength.data + 8; // update local udp packet length
				ipLength.all = ipLength.header + udpLength.all; // update IP Length
				ppp.ip->lengthR = htons(ipLength.all); // update IP length in buffer
				ppp.udp->lengthR = htons(udpLength.all); // update UDP length in buffer
				ppp.pkt.len = ipLength.all + 2 + 4; // update ppp packet length
				IpHeaderCheckSum();  // refresh IP header checksum
				checkSumPseudoHeader(udpLength.all); // get the UDP pseudo-header checksum
				ppp.udp->checksumR = 0; // before TCP checksum calculations the checksum bytes must be set cleared
				unsigned int pseudoHeaderSum = dataCheckSum(ppp.udpStart, udpLength.all, 0); // continue the TCP checksum on the whole TCP packet
				ppp.udp->checksumR = htons(pseudoHeaderSum); // tcp checksum done, store it in the TCP header
				sendPppFrame(); // send the UDP message back
			}
			else if (testFound) {
				unsigned int sI = ntohl(ppp.ip->srcAdrR);
				unsigned int dI = ntohl(ppp.ip->dstAdrR);
				unsigned int sp = ntohs(ppp.udp->srcPortR);
				unsigned int dp = ntohs(ppp.udp->dstPortR);
				int n = sprintf(ppp.pkt.buf + 200, "Response Count %d\n", ppp.responseCounter);
				sendUdp(dI, sI, dp, sp, ppp.pkt.buf + 200, n); // build a udp packet from the ground up
			}
		}
	}

	/// handle a PING ICMP (internet control message protocol) packet
	void ICMPpacket()   // internet control message protocol
	{
		struct {
			unsigned int all; // length of entire ip packet
			unsigned int header; // length of ip header
		} ipLength;
		struct {
			unsigned int all; // length of entire udp packet
			unsigned int data; // length of udp data segment
		} icmpLength;
		ipLength.all = ntohs(ppp.ip->lengthR);  // length of ip packet
		ipLength.header = 4 * ppp.ip->headerLength; // length of ip header
		ppp.icmpStart = ppp.ipStart + ipLength.header; // calculate start of udp header
		icmpLength.all = ipLength.all - ipLength.header; // length of icmp packet
		icmpLength.data = icmpLength.all - 8; // length of icmp data
#define ICMP_TYPE_PING_REQUEST 8
		if (ppp.icmp->type == ICMP_TYPE_PING_REQUEST) {
			ppp.ip->ttl--; // decrement time to live (so we have to update header checksum)
			swapIpAddresses(); // swap the IP source and destination addresses
			IpHeaderCheckSum();  // new ip header checksum (required because we changed TTL)
#define ICMP_TYPE_ECHO_REPLY 0
			ppp.icmp->type = ICMP_TYPE_ECHO_REPLY; // icmp echo reply
			ppp.icmp->checkSumR = 0; // zero the checksum for recalculation
			unsigned int sum = dataCheckSum(ppp.icmpStart, icmpLength.all, 1); // icmp checksum
			ppp.icmp->checkSumR = ntohs(sum); // save big-endian icmp checksum

			int printSize = icmpLength.data; // exclude size of icmp header
			if (printSize > 10) printSize = 10; // print up to 20 characters
			sendPppFrame(); // reply to the ping
		}
	}

	/// handle an incoming TCP packet
	/// use the first few bytes to figure out if it's a websocket, an http request or just pure incoming TCP data
	void tcpHandler()
	{
		int packetLengthIp = ntohs(ppp.ip->lengthR); // size of ip packet
		int headerSizeIp = 4 * ppp.ip->headerLength;  // size of ip header
		ppp.tcpStart = ppp.ipStart + headerSizeIp; // calculate TCP header start
		int tcpSize = packetLengthIp - headerSizeIp; // tcp size = size of ip payload
		int headerSizeTcp = 4 * (ppp.tcp->offset); // tcp "offset" for start of data is also the header size
		char * tcpDataIn = ppp.tcpStart + headerSizeTcp; // start of data TCP data after TCP header
		int tcpDataSize = tcpSize - headerSizeTcp; // size of data block after TCP header

		unsigned int seq_in = ntohl(ppp.tcp->seqTcpR);
		unsigned int ack_in = ntohl(ppp.tcp->ackTcpR);
		unsigned int ack_out = seq_in + tcpDataSize;
		unsigned int seq_out = ack_in; // use their version of our current sequence number

									   // first we shorten the TCP response header to only 20 bytes. This means we ignore all TCP option requests
		headerSizeIp = 20;
		ppp.ip->headerLength = headerSizeIp / 4; // ip header is 20 bytes long
		ppp.ip->lengthR = htonl(40); // 20 ip header + 20 tcp header
									 //tcpSize = 20; // shorten total TCP packet size to 20 bytes (no data)
		headerSizeTcp = 20; // shorten outgoing TCP header size 20 bytes
		ppp.tcpStart = ppp.ipStart + headerSizeIp; // recalc TCP header start
		ppp.tcp->offset = (headerSizeTcp / 4);
		char * tcpDataOut = ppp.tcpStart + headerSizeTcp; // start of outgoing data

		int dataLen = 0; // most of our responses will have zero TCP data, only a header
		int flagsOut = TCP_FLAG_ACK; // the default case is an ACK packet

		ppp.tcp->windowR = htons(1200); // set tcp window size to 1200 bytes

										  // A sparse TCP flag interpreter that implements stateless TCP connections

		switch (ppp.tcp->flag.All) {
		case TCP_FLAG_SYN:
			flagsOut = TCP_FLAG_SYN | TCP_FLAG_ACK; // something wants to connect - acknowledge it
			seq_out = seq_in + 0x10000000U; // create a new sequence number using their sequence as a starting point, increase the highest digit
			ack_out++; // for SYN flag we have to increase the sequence by 1
			break;
		case TCP_FLAG_ACK:
		case TCP_FLAG_ACK | TCP_FLAG_PSH:
			if ((ppp.tcp->flag.All == TCP_FLAG_ACK) && (tcpDataSize == 0)) return; // handle zero-size ack messages by ignoring them
			if ((strncmp(tcpDataIn, "GET /", 5) == 0)) { // check for an http GET command
				flagsOut = TCP_FLAG_ACK | TCP_FLAG_PSH; // we have data, set the PSH flag
			}
			else {
				dataLen = 0;//tcpResponse(tcpDataOut, tcpDataSize, &flagsOut); // not an http GET, handle as a tcp connection
				if (dataLen > 0) flagsOut = TCP_FLAG_ACK | TCP_FLAG_PSH; // if we have any data set the PSH flag
			}
			break;
		case TCP_FLAG_FIN:
		case TCP_FLAG_FIN | TCP_FLAG_ACK:
		case TCP_FLAG_FIN | TCP_FLAG_PSH | TCP_FLAG_ACK:
			flagsOut = TCP_FLAG_ACK | TCP_FLAG_FIN; // set outgoing FIN flag to ask them to close from their side
			ack_out++; // for FIN flag we have to increase the sequence by 1
			break;
		default:
			return; // ignore all other packets
		} // switch

		  // The TCP flag handling is now done
		  // first we swap source and destination TCP addresses and insert the new ack and seq numbers
		swapIpAddresses(); // swap IP source and destination addresses
		swapIpPorts(); // swap IP  source and destination ports

		ppp.tcp->ackTcpR = htonl(ack_out); // byte reversed - tcp/ip messages are big-endian (high byte first)
		ppp.tcp->seqTcpR = htonl(seq_out); // byte reversed - tcp/ip messages are big-endian (high byte first)

		ppp.tcp->flag.All = flagsOut; // update the TCP flags

									  // recalculate all the header sizes
		tcpSize = headerSizeTcp + dataLen; // tcp packet size
		int newPacketSize = headerSizeIp + tcpSize; // calculate size of the outgoing packet
		ppp.ip->lengthR = htons(newPacketSize);
		ppp.pkt.len = newPacketSize + 4 + 2; // ip packet length + 4-byte ppp prefix (ff 03 00 21) + 2 fcs (crc) bytes bytes at the end of the packet

											 // the header is all set up, now do the IP and TCP checksums
		IpHeaderCheckSum(); // calculate new IP header checksum
		checkSumPseudoHeader(tcpSize); // get the TCP pseudo-header checksum
		ppp.tcp->checksumR = 0; // before TCP checksum calculations the checksum bytes must be set cleared
		unsigned int pseudoHeaderSum = dataCheckSum(ppp.tcpStart, tcpSize, 0); // continue the TCP checksum on the whole TCP packet
		ppp.tcp->checksumR = htons(pseudoHeaderSum); // tcp checksum done, store it in the TCP header

		
		sendPppFrame(); // All preparation complete - send the TCP response
		memset(ppp.pkt.buf + 44, 0, 500); // flush out traces of previous data that we may scan for
	}

	/// handle an incoming TCP packet
	void TCPpacket()
	{
		tcpHandler();
	}


	/// handle an IGMP (internet group managment protocol) packet (by ignoring it)
	void IGMPpacket()
	{
		// debugPrintf("IGMP type=%d \n", ppp.pkt.buf[28]);
	}

	/// handle the remaining IP protocols by ignoring them
	void otherProtocol()
	{
		//debugPrintf("Other IP protocol");
	}

	/// process an incoming IP packet
	void IPframe()
	{
		int protocol = ppp.ip->protocol;
		switch (protocol) {
		case    1:
			ICMPpacket();
			break;
		case    2:
			IGMPpacket();
			break;
		case   17:
			UDPpacket();
			break;
		case    6:
			TCPpacket();
			break;
		default:
			otherProtocol();
		}
	}

	/// respond to LCP (line configuration protocol) configuration request) by allowing no options
	void LCPconfReq()
	{
		//debugPrintf("LCP Config ");
		if (ppp.lcp->lengthR != htons(4)) {
			ppp.lcp->code = 4; // allow only "no options" which means Maximum Receive Unit (MRU) is default 1500 bytes
			//debugPrintf("Reject\n");
			sendPppFrame();
		}
		else {
			ppp.lcp->code = 2; // ack zero conf
			//debugPrintf("Ack\n");
			sendPppFrame();
			//debugPrintf("LCP Ask\n");
			ppp.lcp->code = 1; // request no options
			sendPppFrame();
		}
	}

	/// handle LCP acknowledge packets by ignoring them
	void LCPconfAck()
	{
		//debugPrintf("LCP Ack\n");
	}

	/// handle LCP end (disconnect) packets by acknowledging them and by setting ppp.online to false
	void LCPend()
	{
		ppp.lcp->code = 6; // end
		sendPppFrame(); // acknowledge
		ppp.online = 0; // start hunting for connect string again
		pppInitStruct(); // flush the receive buffer
		//debugPrintf("LCP End (Disconnect from host)\n");
	}

	/// respond to other LCP requests by ignoring them
	void LCPother()
	{
		//debugPrintf("LCP Other\n");
	}

	/// process incoming LCP packets
	void LCPframe()
	{
		int code = ppp.lcp->code;
		switch (code) {
		case 1:
			LCPconfReq();
			break; // config request
		case 2:
			LCPconfAck();
			break; // config ack
		case 5:
			LCPend();
			break; // end connection
		default:
			LCPother();
		}
	}

	/// discard packets that are not IP, IPCP, or LCP
	void discardedFrame()
	{
		//debugPrintf("Frame is not IP, IPCP or LCP: %02x %02x %02x %02x\n", ppp.pkt.buf[0], ppp.pkt.buf[1], ppp.pkt.buf[2], ppp.pkt.buf[3]);
	}

	/// determine the packet type (IP, IPCP or LCP) of incoming packets
	void determinePacketType()
	{
		if (ppp.ppp->address != 0xff) {
			//debugPrintf("Unexpected: PPP address != ff\n");
			return;
		}
		if (ppp.ppp->control != 3) {
			//debugPrintf("Unexpected: PPP control !=  3\n");
			return;
		}
		unsigned int protocol = ntohs(ppp.ppp->protocolR);
		switch (protocol) {
		case 0xc021:
			LCPframe();
			break;  // link control
		case 0x8021:
			IPCPframe();
			break;  // IP control
		case 0x0021:
			IPframe();
			break;  // IP itself
		default:
			discardedFrame();
		}
	}