<?php

//var_dump(blacknc_array());
//blacknc_loaded_functions();
$fp = blacknc_fopen("/tmp/log", "rw");
var_dump($fp);
unset($fp);
