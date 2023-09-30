#!/bin/bash

BWCMD=./q_bandwidth

if [[ -z "$1" ]] ; then
	if [[ ! -x "${BWCMD}" && -x ./build/${BWCMD} ]]; then
		BWCMD="./build/${BWCMD}"
	fi
elif [[ ! -z "$1" ]] ; then
        BWCMD="$1"
fi
if [[ ! -x "${BWCMD}" ]] ; then
        echo "error: executable program '${BWCMD}' not found"
        exit 1
fi

XDATE=$(date '+%Y%m%d-%H%M%S')
REPDIR="."
if [[ -d reports ]]; then
        REPDIR=reports
fi
REPORTFILE=${REPDIR}/q-bw-report.$XDATE.txt

(
echo "Queue bandwidth report"
echo "Date: ${XDATE}"
echo "Start at: " $(date)
uname -a
lscpu
numactl -H
numactl -s

CPUCOUNT=$(lscpu | awk '/^CPU\(s\):/{print $2}')
MILLISECONDS=400; # 20 100 1000
DEPTHS=( 1 2 4 8 64 )
PRODUCERS=( 1 2 4 8 16 )
CONSUMERS=( 1 2 4 8 16 )
DATASIZES=( 4 8 12 )
INDXSIZES=( 4 8 )
BOOSTQ=" -b ";
if ! ${BWCMD} -m 20 -b > /dev/null 2>&1 ;then
	BOOSTQ=""
fi


loopn=0

for indsize in ${INDXSIZES[*]};
do 
    for datasize in ${DATASIZES[*]};
    do
	    if (( ( $indsize + $datasize ) <= 16 )) ; then
            for d in ${DEPTHS[*]};
            do
                    for p in ${PRODUCERS[*]};
                    do
                        for c in ${CONSUMERS[*]};
                        do
				if [[ $(( $p + $c )) -gt $CPUCOUNT ]]; then continue ; fi
                                echo "$loopn"
				if [[ ! -z "$BOOSTQ" && $indsize -eq 4 ]]; then
                                   ${BWCMD} -m ${MILLISECONDS} ${BOOSTQ} -w $indsize -W $datasize -p $p -c $c -d $d
				fi
                                ${BWCMD} -m ${MILLISECONDS} -w $indsize -W $datasize -p $p -c $c -d $d
                                (( loopn++ ))
                        done
                    done
            done
        fi
    done
done
echo "End at: " $(date)
) 2>&1 | tee ${REPORTFILE}

if [[ -x /usr/bin/perl && -x ./scripts/report-processing.pl ]] ; then
    ./scripts/report-processing.pl ${REPORTFILE} >> ${REPORTFILE}
fi

echo "report file: ${REPORTFILE}"

exit 0
