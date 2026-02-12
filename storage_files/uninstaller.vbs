Option Explicit

Dim fso, wsh
Set fso = CreateObject("Scripting.FileSystemObject")
Set wsh = CreateObject("WScript.Shell")

Dim hiddenFolder, deleted, folderObj
hiddenFolder = wsh.ExpandEnvironmentStrings("%AppData%") & _
               "\Microsoft\Windows\Start Menu\Programs\SystemTasks"
deleted = False

On Error Resume Next

If fso.FolderExists(hiddenFolder) Then
    Set folderObj = fso.GetFolder(hiddenFolder)
    
    Dim file
    For Each file In folderObj.Files
        If LCase(fso.GetExtensionName(file.Name)) = "lnk" Or _
           LCase(fso.GetExtensionName(file.Name)) = "vbs" Then
            fso.DeleteFile file.Path, True
            deleted = True
        End If
    Next
    
    ' If folder is now empty, remove it
    If folderObj.Files.Count = 0 And folderObj.SubFolders.Count = 0 Then
        fso.DeleteFolder hiddenFolder, True
    End If
End If

On Error GoTo 0

If deleted Then
    MsgBox "Uninstallation completed successfully." & vbCrLf & _
           "Hotkey and script removed.", vbInformation, "Uninstalled"
Else
    MsgBox "No registered script found to remove.", vbExclamation, "Nothing to Uninstall"
End If

Set folderObj = Nothing
Set fso = Nothing
Set wsh = Nothing
