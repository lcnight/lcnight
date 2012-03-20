#! /usr/bin/php5 -q
<?php 
for ($iixx = 0 ; $iixx < 3 ; ++$iixx) {
    fwrite(STDOUT, 'from STDOUT'."\n");
}

$str = fgets(STDIN);
fwrite(STDERR, 'get from STDIN'.$str);
?>
