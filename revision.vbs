ret = MsgBox("Do you want to update revision.h ?", vbYesNo + vbDefaultButton2 + vbQuestion, "Pre-Build Event :: Updating version")

If ret = vbYes Then
    Set shell = WScript.CreateObject("WScript.Shell")
    code = shell.Run("revision.bat", 1, true)
    If code <> 0 Then
        MsgBox "The script exited with code: " & code, vbOKOnly + vbExclamation, "Pre-Build Event :: Updating version"
    End If
End If
