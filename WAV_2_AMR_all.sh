#!/bin/bash

BIN_SOX=./sox
BIN_ENC=./pcm2amr_enc
BIN_SCP=scp

HOST=10.1.105.20
DIR=/lcment/media

wav2pcm()
{
	INFILE=$1
	RATE=$2
	OUTFILE=$3

	${BIN_SOX} ${INFILE} --rate ${RATE} --bit 16 -c 1 -e signed-integer -t raw ${OUTFILE}
}

pcm2amr()
{
	INFILE=$1
	CODEC=$2
	MODE=$3
	OUTFILE=$4

	# pcm2amr_enc genrate infilename.amr for infilename
	${BIN_ENC} ${INFILE} ${CODEC} ${MODE}
	#${BIN_SCP} ${INFILE}.amr ktms@${HOST}:${DIR}/${OUTFILE}
	mv ${INFILE}.amr ${OUTFILE}
}

convert_all_amr()
{
	FILE=$1

	if [ ! -f ${FILE} ] ; then
		echo "$FILE not exist"
		exit 0
	fi

	BASENAME=`echo ${FILE} | cut -d'.' -f 1`
	echo "convert_all_amr ${BASENAME}"

	# AMR-NB
	MODE_SET=0
	RATE="8k"
	CODEC="nb"
	wav2pcm ${FILE} ${RATE} ${BASENAME}.pcm
	for MODE_STR in 4.75 5.15 5.9 6.7 7.4 7.95 10.2 12.2
	do
		pcm2amr ${BASENAME}.pcm ${CODEC} ${MODE_SET} ${BASENAME}.AMR.${MODE_STR}.amr >> /dev/null
		echo "${BASENAME}.AMR.${MODE_STR}.amr"
		let "MODE_SET += 1"
	done

	# AMR-WB
	MODE_SET=0
	RATE="16k"
	CODEC="wb"
	wav2pcm ${FILE} ${RATE} ${BASENAME}.pcm16
	for MODE_STR in 6.6 8.85 12.65 14.25 15.85 18.25 19.85 23.05 23.85
	do
		pcm2amr ${BASENAME}.pcm16 ${CODEC} ${MODE_SET} ${BASENAME}.AMR-WB.${MODE_STR}.amr >> /dev/null
		echo "${BASENAME}.AMR-WB.${MODE_STR}.amr"
		let "MODE_SET += 1"
	done
}

convert_all_amr $1

# end of script.
