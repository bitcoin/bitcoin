Enter file contents here;the following is an infection scan of the key
;autoit is used for this project
;http://www.autoitscript.com
;***
;author information***
;Kevin J. Sisco(kjsisco)
;kevinsisco61784@gmail.com
;***
;begin code***
If FileGetSize("key.out") < 16 Then
;assume key is infected
FileRecycle("key.out")
;add to recycle bin
Sleep(100)
;wait 1 ms
FileRecycleEmpty()
;empty recycle bin
EndIf
If FileGetSize("key.out") > 16 Then
;again, it can't be trusted
FileRecycle("key.out")
Sleep(100)
FileRecycleEmpty()
EndIf
