#!/usr/bin/php -q

<?php

if (count($argv) < 3) {
    echo "Usage: sms <content> <phones...>\n";
    exit(1);
}
array_shift($argv);

require_once('nusoaplib/nusoap.php');

$url = 'http://sdkhttp.eucp.b2m.cn/sdk/SDKService?wsdl';
$timeout = 5;
$response_timeout = 30;

$proxyhost = false;
$proxyport = false;
$proxyusername = false;
$proxypassword = false; 

$soap = new nusoap_client($url,false,$proxyhost,$proxyport,$proxyusername,$proxypassword,$timeout,$response_timeout); 
$soap->soap_defencoding = "UTF-8";
$soap->decode_utf8 = false;	

$namespace = 'http://sdkhttp.eucp.b2m.cn/';

$serialNumber = '';
$sessionKey = '';

$sendTime = '';
$addSerial = '';
$charset = 'GBK';
$priority = 5;

$content = $argv[0];
array_shift($argv);

$params = array(
    'arg0'=>$serialNumber
    , 'arg1'=>$sessionKey
    , 'arg2'=>$sendTime
    , 'arg4'=>$content
    , 'arg5'=>$addSerial
    , 'arg6'=>$charset
    , 'arg7'=>$priority
);

foreach($argv as $mobile)
{
    array_push($params,new soapval("arg3",false,$mobile));	
}

$result = $soap->call("sendSMS",$params,$namespace);

return $result;

# sn = '3SDK-EMY-0130-PEURL';
# sk = '3fc2719ce96ebdf42b6dc1a1d8303e1b';
?>

