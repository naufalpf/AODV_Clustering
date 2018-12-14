# ===================================================================
# AWK Script for calculating: 
#     => Average End-to-End Delay.
# ===================================================================

BEGIN {
  seqno = -1;    
#    droppedPackets = 0; 
#    receivedPackets = 0; 
  count = 0;
}

{
  if($4 == "AGT" && $1 == "s" && seqno < $6) {
    seqno = $6;
  } 
#	else if(($4 == "AGT") && ($1 == "r")) {
#            receivedPackets++;
#    } else if ($1 == "D" && $7 == "tcp" && $8 > 512){
#            droppedPackets++;            
#  } 

  #end-to-end delay
  if($4 == "AGT" && $1 == "s") {
    start_time[$6] = $2;
  } else if(($7 == "cbr") && ($1 == "r")) {
    end_time[$6] = $2;
  } else if($1 == "D" && $7 == "cbr") {
    end_time[$6] = -1;
  } 
}


END {        
  for(i=0; i<=seqno; i++) {
    if(end_time[i] > 0) {
      delay[i] = end_time[i] - start_time[i];
      count++;
    }
    else {
      delay[i] = -1;
    }
  }

  for(i=0; i<=seqno; i++) {
    if(delay[i] > 0) {
      n_to_n_delay = n_to_n_delay + delay[i];
    }         
  }

  # n_to_n_delay = n_to_n_delay/count;

  print "\n";

  print "GeneratedPackets \t= " seqno+1;
  print "ReceivedPackets  \t= " receivedPackets;
  print "Packet Delivery Ratio \t= " receivedPackets/(seqno+1)*100 "%";
  print "Total Dropped Packets \t= " droppedPackets;
  print "Average End-to-End Delay \t= " n_to_n_delay * 1000 " ms";
  print "\n";
} 
