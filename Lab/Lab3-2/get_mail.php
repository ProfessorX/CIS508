<?php
 
function checkGmail($username, $password)
{
    $url = "https://mail.google.com/mail/feed/atom"; 
 
    $curl = curl_init();
    curl_setopt($curl, CURLOPT_URL, $url);
    // You may need to comment out the next line
    curl_setopt($curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($curl, CURLOPT_USERPWD, $username . ":" . $password);
    curl_setopt($curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_setopt($curl, CURLOPT_ENCODING, "");
    $mailData = curl_exec($curl);
    curl_close($curl);
 
    return $mailData;
}
 
header('Content-Type:text/xml; charset=UTF-8');
$feed = checkGmail("arduino.cis508@gmail.com", "arduino2014");
echo $feed; //return xml formatted data

>