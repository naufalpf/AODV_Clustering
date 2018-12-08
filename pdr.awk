BEGIN {
        sendLine = 0;
        recvLine = 0;
        fowardLine = 0;
	TC =0;
	rt_pkts=0;
	rt_send=0;
	rt_forward=0;
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

$0 ~/^s.* \[TC / {
        TC ++ ;
}



{  
	if($4 == "AGT" && $1 == "s" && seqno < $6) {

          seqno = $6;

	} 
	#end-to-end delay

	    if($4 == "AGT" && $1 == "s") {

		  start_time[$6] = $2;

	    } else if(($7 == "cbr") && ($1 == "r")) {

		end_time[$6] = $2;

	    } else if($1 == "D" && $7 == "cbr") {

		  end_time[$6] = -1;

	    } else if (($1 == "s" || $1 == "f") && ($4 == "RTR") && ($7 == "AODV")) {
    			
		  rt_pkts++;
	    }
	      if (($1 == "s") && ($4 == "RTR") && ($7 == "AODV") && ($25 == "(REQUEST)")) {
    			
		  rt_send++;
	    }
	      if (($1 == "s") && ($4 == "RTR") && ($7 == "AODV") && ($25 == "(REQUEST)") && ($3 != "_58_")) {
	    			
			  rt_forward++;
	    }

}

 
END {        
  
    for(i=0; i<=seqno; i++) {

          if(end_time[i] > 0) {

              delay[i] = end_time[i] - start_time[i];

                  count++;

        }

            else

            {

                  delay[i] = -1;

            }

    }

    for(i=0; i<=seqno; i++) {

          if(delay[i] > 0) {

              n_to_n_delay = n_to_n_delay + delay[i];

        }         

    }

	n_to_n_delay = n_to_n_delay/count;

	#printf "Packet sendLine \t= %d \n", sendLine;
	#printf "Packet recvLine \t= %d \n", recvLine;
	#printf "Packet PDR Ratio \t= %.4f \n", (recvLine/sendLine);
	#printf "Packet loss 	\t= %d \n", (sendLine-recvLine);
	#printf "Packet forwardLine\t= %d \n", fowardLine;
	#printf "End-to-End Delay \t= " n_to_n_delay * 1000 " ms \n";
	#printf "Topology Control \t= %d \n", TC;
	#printf "Routing Packets \t= %d \n", rt_pkts;
	#printf "Route Request Send \t= %d \n", rt_send;
	printf  sendLine"\t"recvLine"\t%.4f\t"(sendLine-recvLine)"\t" n_to_n_delay * 1000" ms\t" rt_pkts"\t"rt_send"\t" rt_forward"\n",(recvLine/sendLine);
}
