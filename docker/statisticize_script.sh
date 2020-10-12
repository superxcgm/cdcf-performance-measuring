#!/bin/bash

IFS_old=$IFS
IFS=$'\n'
enqueueing_avg=0
dequeueing_avg=0
initiation_avg=0
single_producer_sending_avg=0
multi_producer_sending_avg=0
max_throughput_avg=0
ping_lantency_avg=0;
ping_throughput_avg=0

enqueueing_count=0
dequeueing_count=0
initiation_count=0
single_producer_sending_count=0
multi_producer_sending_count=0
max_throughput_count=0
ping_lantency_count=0;
ping_throughput_count=0

while read LINE
do
	label=`echo $LINE | sed 's@[[:space:]]@@g' | sed 's@[:>]@@g'`
	#echo $label
	read LINE
	read LINE

	if [ "$label" = "Pinglatency" ]
	then
		ops_per_second=`echo $LINE | sed 's@[ns]@@g' | sed 's@[[:space:]]@@g'`
		ping_lantency_avg=`expr ${ping_lantency_avg} + ${ops_per_second}`
		let ping_lantency_count++
		for (( i = 0; i < 7; i++ ));
		do
			read LINE
		done
		continue
	else
		read LINE
	fi

	ops_per_second=`echo $LINE | sed 's@[ops/s]@@g' | sed 's@[[:space:]]@@g'`
	#echo $ops_per_second

	case $label in
		"Enqueueing")
			enqueueing_avg=`expr ${enqueueing_avg} + ${ops_per_second}`
			let enqueueing_count++
		;;
		"Dequeueing")
			dequeueing_avg=`expr ${dequeueing_avg} + ${ops_per_second}`
			let dequeueing_count++
		;;
		"Initiation")
			initiation_avg=`expr ${initiation_avg} + ${ops_per_second}`
			let initiation_count++
		;;
		"Single-producersending")
			single_producer_sending_avg=`expr ${single_producer_sending_avg} + ${ops_per_second}`
			let single_producer_sending_count++
		;;
		"Multi-producersending")
			multi_producer_sending_avg=`expr ${multi_producer_sending_avg} + ${ops_per_second}`
			let multi_producer_sending_count++
		;;
		"Maxthroughput")
			max_throughput_avg=`expr ${max_throughput_avg} + ${ops_per_second}`
			let max_throughput_count++
		;;
		"Pingthroughput")
			read LINE
			ops_per_second=`echo $LINE | sed 's@[ops/s]@@g' | sed 's@[[:space:]]@@g'`
			ping_throughput_avg=`expr ${ping_throughput_avg} + ${ops_per_second}`
		let ping_throughput_count++
	esac

done  < out.txt

echo "------------ SUM -----------"
echo "Enqueueing: $enqueueing_avg opt/s"
echo "Dequeueing: $dequeueing_avg opt/s"
echo "Initiation: $initiation_avg opt/s"
echo "Single-producersending: $single_producer_sending_avg opt/s"
echo "Multi-producersending: $multi_producer_sending_avg opt/s"
echo "Maxthroughput: $max_throughput_avg opt/s"
echo "Pinglatency: $ping_lantency_avg opt/s"
echo "Pingthroughput: $ping_throughput_avg opt/s"
echo ""
echo "------------ AVG -----------"
echo "Enqueueing: `expr ${enqueueing_avg} / ${enqueueing_count}` opt/s"
echo "Dequeueing: `expr ${dequeueing_avg} / ${dequeueing_count}` opt/s"
echo "Initiation: `expr ${initiation_avg} / ${initiation_count}` opt/s"
echo "Single-producersending: `expr ${single_producer_sending_avg} / ${single_producer_sending_count}` opt/s"
echo "Multi-producersending: `expr ${multi_producer_sending_avg} / ${multi_producer_sending_count}` opt/s"
echo "Maxthroughput: `expr ${max_throughput_avg} / ${max_throughput_count}` opt/s"
echo "Pinglatency: `expr ${ping_lantency_avg} / ${ping_lantency_count}` opt/s"
echo "Pingthroughput: `expr ${ping_throughput_avg} / ${ping_throughput_count}` opt/s"
# for line in  `cat out.txt`
# do

# done;

IFS=$IFS_old