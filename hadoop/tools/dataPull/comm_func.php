<?php
//array[time][id] = value;
function timeid_arr_sort_(&$timeidsvalue)
{
    $sortResults = array();

    $times = array_keys($timeidsvalue);
    $times_cnt = count($times);
    arsort($timeidsvalue[$times[$times_cnt - 1]]);
    foreach($timeidsvalue[$times[$times_cnt - 1]] as $id => $value) {
        for($i = 0; $i < $times_cnt; ++$i) {
            $timeTok = $times[$i];
            if (!isset($sortResults[$timeTok])) {
                $sortResults[$timeTok] = array();
            }
            $sortResults[$timeTok][$id] = $timeidsvalue[$timeTok][$id];
        }
    }
    $timeidsvalue = $sortResults;
}

function timeid_arr_topn_(&$timeidsvalue, $topn = 11)
{
    $times = array_keys($timeidsvalue);
    $times_cnt = count($times);
    if (count($timeidsvalue[$times[$times_cnt - 1]]) <= $topn) return;

    $lastArrKeys = array_keys($timeidsvalue[$times[$times_cnt - 1]]);
    $topnArr = array();
    foreach($times as $time) {
        if (!isset($topnArr[$time])) {
            $topnArr[$time] = array();
        }

        $i = 0;
        $otherSum = 0;
        foreach($lastArrKeys as $id) {
            if($i < $topn) {
                $topnArr[$time][$id] = $timeidsvalue[$time][$id];
            } else {
                $otherSum += $timeidsvalue[$time][$id];
            }
            ++$i;
        }
        $topnArr[$time]['other'] = $otherSum;
    }
    $timeidsvalue = $topnArr;
}

function timeid_arr_print_(&$timeidsvalue)
{
    $times = array_keys($timeidsvalue);
    $times_cnt = count($times);
    $lastArrKeys = array_keys($timeidsvalue[$times[$times_cnt - 1]]);

    echo "time\t";
    foreach($times as $time) {
        echo $time . "\t";
    }
    echo "\n";

    foreach($lastArrKeys as $id) {
        echo "$id\t";
        foreach($times as $time) {
            echo $timeidsvalue[$time][$id] . "\t";
        }
        echo "\n";
    }
}

?>
