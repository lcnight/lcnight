<?php
//require_once('./phpanalysis.class.php');
$argv = $_SERVER['argv'];
$page_num = $argv[1];
$keywords = '';

$i = 0;
$keywords = '';
foreach($argv as $v) {
    if ($i < 2) {
        $i++;
        continue;
    }
    $i++;
    $keywords .= ' '. $v;
}

//trim the keywords
$trimmed_keywords = trim($keywords);
$keyword_ary = explode(" ", $trimmed_keywords);
//
print_r($keyword_ary);


$array = array('cmd_id' => 2003,
               'page_num' => $page_num,
               'result_per_page' => 1,
               'keywords' => $keyword_ary);

$json_array = json_encode($array) . "\r\n";

echo "strlen: ", strlen($json_array), "\n";

echo $json_array, "\n";

$pid_ary = array();

for ($i = 0; $i < 30; $i++) {
    $pid = pcntl_fork();
    if ($pid == -1) {
        exit();
    } else if ($pid) {
        $pid_ary[] = $pid;
        continue;
    } else {
        $socket = socket_create(AF_INET,SOCK_STREAM,SOL_TCP);
        if(!$socket)
        {
            echo("create socket failed!!!\n");
            exit();
        }

        if(!socket_connect($socket, "10.1.8.136", 12601))
        {
            echo("connect to server failed!!!\n");
            exit();
        }

        $result = socket_write($socket,$json_array,strlen($json_array));
        if($result === false)
        {
            echo("send data to server failed!!!\n");
            exit();
        }

        if($result != strlen($json_array))
        {
            echo("unable to send all data!!!\n");
            exit();
        }

        //recv from server
        $recved_string = socket_read($socket,16 * 1024,PHP_BINARY_READ);
        if($recved_string == false || strlen($recved_string) <= 0)
        {
            echo("recved data from server failed!!!!\n");
            exit();
        }

        //echo $recved_string, "\n";

        $rcv_array = json_decode($recved_string, true);
        echo($rcv_array);

        echo "status ", $rcv_array['status'], "\n";
        if ($rcv_array['status'] != 0) {
            exit();
        }

        echo "search_keywords : ";
        foreach($rcv_array['search_keywords'] as $k) {
            echo "\033[1;40;31m" . $k . "\033[0m ";
        }
        echo "\n";

        echo "search_result_num: " . "\033[1;40;32m" . $rcv_array['search_result_num'] . "\033[0m", "\n";

        foreach($rcv_array['snippet'] as $snippet) {
            echo "snippet_url: \033[1;40;31m" . $snippet['snippet_url'] . "\033[0m", "\n";

            if (array_key_exists("title_offset", $snippet)) {
                $color_ary = array();
                foreach($snippet['title_offset'] as $k => $v) {
                    foreach($v as $offset) {
                        @$color_ary["$offset"] .= "\033[1;40;32m";
                        $offset_end = $offset + mb_strlen($k, "utf-8");
                        @$color_ary["$offset_end"] .= "\033[0m";
                    }
                }
                echo "snippet title: ";
                for ($i = 0; $i < mb_strlen($snippet['snippet_title']); $i++) {
                    if (isset($color_ary["$i"])) {
                        echo $color_ary["$i"];
                    }
                    $str = mb_substr($snippet['snippet_title'], $i, 1, "utf-8");
                    echo $str;
                }
                echo "\n";

            } else {
                echo "snippet title: ", $snippet['snippet_title'], "\n";
            }

            if (array_key_exists("content_offset", $snippet)) {
                $color_ary = array();
                foreach($snippet['content_offset'] as $k => $v) {
                    foreach($v as $offset) {
                        @$color_ary["$offset"] .= "\033[1;40;32m";
                        $offset_end = $offset + mb_strlen($k, "utf-8");
                        @$color_ary["$offset_end"] .= "\033[0m";
                    }
                }
                echo "snippet content: ";
                for ($i = 0; $i < mb_strlen($snippet['snippet_content']); $i++) {
                    if (isset($color_ary["$i"])) {
                        echo $color_ary["$i"];
                    }
                    $str = mb_substr($snippet['snippet_content'], $i, 1, "utf-8");
                    echo $str;
                }
                echo "\n";

            } else {
                echo "snippet content: ", $snippet['snippet_content'], "\n";
            }

            echo "\n\n";
        }
    }
}

for ($i = 0; $i < 30; $i++) {
    pcntl_waitpid(@$pid_ary[$i], $status, WNOHANG);
    usleep(100);
}

?>
