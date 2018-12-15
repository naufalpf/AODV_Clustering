# ===================================================================
# AWK Script for calculating: 
#     => Packet Delivery Ratio
# ===================================================================

BEGIN {
  sendLine = 0;
  recvLine = 0;
  fowardLine = 0;
}
 
$0 ~/^s.* AGT/ {
  sendLine ++ ;
}
 
$0 ~/^r.* AGT/ {
  recvLine ++ ;
}
 
$0 ~/^f.* RTR/ {
  fowardLine ++ ;
}
 
END {
  # printf "SendLine : %d \nRecvLine : %d \nPacket Delivery Ratio : %.4f \nForwardLine : %d \n", sendLine, recvLine, (recvLine/sendLine), fowardLine;

  # PRINT RESULT
	printf "================================================= \n"
	printf "Count Packet Delivery Ratio from aomdv-result.tr \n"
	printf "================================================= \n"
	printf "Packet SendLine \t= %d \n", sendLine;
	printf "Packet RecvLine \t= %d \n", recvLine;
	printf "Packet Loss 	\t= %d \n", (sendLine-recvLine);
	printf "Packet ForwardLine\t= %d \n", fowardLine;
	printf "Packet Delivery Ratio \t= %.4f \n", (recvLine/sendLine);
	printf "\n"
}
 