#! /bin/bash

if [[ $# != 2 ]]; then
    echo "usage: `basename $0` <channel|region> gameid"
    exit
fi

type=$1
gameid=$2

case $type in
    channel)
    ./list-channel.php $gameid newers 2011{05,06,07,08,09,10,11,12} 20120{1,2,3,4,5} | tr '\t' ',' > newer.csv
    ./list-channel.php $gameid keepers 2011{05,06,07,08,09,10,11,12} 20120{1,2,3,4,5} | tr '\t' ',' > keeper.csv
    ./list-channel.php $gameid backers 2011{05,06,07,08,09,10,11,12} 20120{1,2,3,4,5} | tr '\t' ',' > backer.csv
    ;;
    region)
    ./list-region.php $gameid newers 2011{05,06,07,08,09,10,11,12} 20120{1,2,3,4,5} | tr '\t' ',' > newer.csv
    ./list-region.php $gameid keepers 2011{05,06,07,08,09,10,11,12} 20120{1,2,3,4,5} | tr '\t' ',' > keeper.csv
    ./list-region.php $gameid backers 2011{05,06,07,08,09,10,11,12} 20120{1,2,3,4,5} | tr '\t' ',' > backer.csv
    ;;
    *)
    echo 'not support'
    ;;
esac
