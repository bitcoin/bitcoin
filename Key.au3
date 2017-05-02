Enter file contents here;the following is a layer of security
;autoit is used for this project
;http://www.autoitscript.com
;***
;author information***
;Kevin J. Sisco(kjsisco)
;kevinsisco61784@gmail.com
;***
;begin code***
$r = Random(0, 1100)
;random number
$i = Inputbox(" ", "Please enter a letter")
If IsNumber($i) Then
;let's check the input
Exit
EndIf
$s = StringToBinary($i)
;convert to binary
If BinaryLen($s) > 1 Then
;keep it to one letter
Exit
EndIf
$x = BitXor($s, $r)
;xor of string and random number
$b = BitRotate($x, 32)
;rotate 32 bits to the left
$k = $s+$x+$b*Log($r)
;key
$f = FileOpen("key.out", 1)
;output stream
FileWriteLine($f, $k*0.50)
;tax of the key
FileFlush($f)
;flush the buffer
FileClose($f)
;close the stream
