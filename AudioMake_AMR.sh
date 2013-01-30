#!/bin/bash

FILE=$1
CODEC=$2	# nb or wb
MODE=$3		# octet-align = 0 or 1

case ${CODEC} in
	nb)
		RATE=8k
		;;
	wb)	
		RATE=16k
		;;
	*)	echo "Usage:${IAM} <PCAP FILE> <nb|wb> <0|1>" ;;
esac

./pcap2amr_dec $FILE.pcap $CODEC $MODE

sox --rate ${RATE} --bit 16 -c 1 -s -t raw $FILE.pcap.pcm $FILE.wav

# end of script.
