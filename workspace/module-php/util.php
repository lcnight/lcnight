<?php 
// define new line in environment
define(nl, "\n");
// run in DEBUG/RELEASE mode
define(DEBUG, true);
// comma separated email receiptor list
define(Recip, 'lc@taomee.com');

function Error($msg) {
    if ($msg == '') { 
        echo nl; return;
    }
    echo date('Y-m-d H:i:s')." [Error] ".$msg.nl;
}
function Info($msg) {
    if ($msg == '') { 
        echo nl; return;
    }
    echo date('Y-m-d H:i:s')." [Info] ".$msg.nl;
}
function Debug($msg) {
    if ($msg == '') { 
        echo nl; return;
    }
    if(DEBUG) echo date('Y-m-d H:i:s')." [Debug] ".$msg.nl;
}


function SendMail($title, $content)
{
    $ll = system("echo \"$content\"| mail -s \"$title\" " . Recip, $retval);
    Msg("return code: $retval($ll)\nsend mail\ntitle: ${title}\nto: ".Recip);
}

function get_dirfilepat($dir, $pat, &$filesarr) 
{
    $dh  = opendir($dir);
    while ($filename = readdir($dh)) {
        if (stristr($filename, $pat)) {
            $filename = "$dir/$filename";
            if (file_used($filename, $pids)) {
                Msg("$filename used by: $pids");
                continue;
            }
            $filesarr[] = $filename;
        }
    }
    closedir($dh);
    sort($filesarr);
}

function file_used(&$file, &$pids='')
{
    $cmd = "fuser $file";
    $ret_str = system($cmd, $ret_code);
    // 0 : used
    // 1 : none
    // >1: fatal error
    if ($ret_code > 1) {
        Msg("System error code: $ret_code, \n\t command: $cmd\n");
        exit($ret_code);
    }

    if (strlen($ret_str) == 0) {
        return false;
    } else {
        $pids = $ret_str;
        return true;
    }
}

function get_num($str) 
{
    $value = 0;
    for ($i = 0; $str[$i] ; ++$i) {
        if (is_numeric($str[$i])) {
            $numstr = substr($str, $i);
            $value = intval($numstr);
            break;
        }
    }
    return $value;
}

function bak_file($bak_dir, $bak_flag, $file) 
{
    //Msg("dir: $bak_dir\n, flag: $bak_flag\n, file: $file");
    if (!file_exists($bak_dir)) { mkdir($bak_dir); }

    $rel_bak_dir = $bak_dir . '/' . $bak_flag;
    if (!file_exists($rel_bak_dir)) { mkdir($rel_bak_dir); }

    $basefname = basename($file);
    Msg("rename($file, $rel_bak_dir/$basefname)");
    return rename($file, $rel_bak_dir.'/'.$basefname);
}

function purge_dir($t_dir, $days) 
{
    $cmd = "find $t_dir -maxdepth 1 -atime +$days -exec rm -rfv {} +;";
    system($cmd, $ret_val);
    return $ret_val;
}

?>
