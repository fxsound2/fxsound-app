Function InstallDFXDriver(path)
    Dim ptDevCon, output
    Dim log
    Const VENDOR_CODE="23"
    
    InstallDFXDriver = False

    If Right(path, 1) <> "\" Then
        path = path & "\"
    End If

    arch = GetObject("winmgmts:root\cimv2:Win32_Processor='cpu0'").AddressWidth

    drvPath = path & "Drivers"
    bootStrapPath = path & "Drivers\bootstrap"
    appsPath = path & "Apps"

    Set wscShell = CreateObject("WScript.Shell")
    Set fileSys = CreateObject("Scripting.FileSystemObject")
    Set log = fileSys.CreateTextFile("dfx.log", True)

    log.WriteLine("Arch=" & CStr(arch))

    If arch = 32 Then
        ptDevCon = "ptdevcon32.exe"
    Else
        ptDevCon = "ptdevcon64.exe"
    End If
   
    wscShell.CurrentDirectory = appsPath
    Set wscExec = wscShell.Exec("DfxSetupDrv.exe check")
    output = wscExec.StdOut.ReadAll
    log.WriteLine("dfx check " & output)
    wscShell.CurrentDirectory = drvPath
    ' Driver already installed could be old
    ' so uninstall before installing the latest driver
    If output = "DFX Audio Enhancer" Then
        Set wscExec = wscShell.Exec(ptDevCon & " remove")
        output = wscExec.StdOut.ReadAll
        log.WriteLine("dfx remove " & output)
        If Len(output) = 0 Then
            log.Close
            Exit Function
        End If
        If InStr(output, "Success") = 0 Then
            log.Close
            Exit Function
        End If

        WScript.Sleep 10000

        wscShell.CurrentDirectory = bootStrapPath
        Set wscExec = wscShell.Exec(ptDevCon & " install")
        output = wscExec.StdOut.ReadAll
        log.WriteLine("dfx install bootstrap " & output)
        If Len(output) = 0 Then
            log.Close
            Exit Function
        End If
        If InStr(output, "Success") = 0 Then
            log.Close
            Exit Function
        End If

        WScript.Sleep 20000

        wscShell.CurrentDirectory = drvPath
        Set wscExec = wscShell.Exec(ptDevCon & " remove")
        output = wscExec.StdOut.ReadAll
        log.WriteLine("dfx remove bootstrap " & output)
        If Len(output) = 0 Then
            log.Close
            Exit Function
        End If
        If InStr(output, "Success") = 0 Then
            log.Close
            Exit Function
        End If

        WScript.Sleep 10000
    End If
   
    Set wscExec = wscShell.Exec(ptDevCon & " install")
    output = wscExec.StdOut.ReadAll
    log.WriteLine("dfx install " & output)
    If Len(output) = 0 Then
        log.Close
        Exit Function
    End If
    If InStr(output, "Success") = 0 Then
        log.Close
        Exit Function
    End If

    WScript.Sleep 5000
    
    wscShell.CurrentDirectory = appsPath
    Set wscExec = wscShell.Exec("DfxSetupDrv.exe getguid")
    output = wscExec.StdOut.ReadAll
    log.WriteLine("dfx guid " & output)
    If Left(output, 1) = "{" And Right(output, 1) = "}" Then
        wscShell.RegWrite "HKLM\Software\DFX\" & VENDOR_CODE & "\devices\dfx_guid", output, "REG_SZ"
    Else
        log.Close
        Exit Function
    End If

    Set wscExec = wscShell.Exec("DfxSetupDrv.exe setname")
    output = wscExec.StdOut.ReadAll
    log.WriteLine("dfx setname " & output)
    If output <> "Success" Then
        log.Close
        Exit Function
    End If

    Set wscExec = wscShell.Exec("DfxSetupDrv.exe seticon")
    output = wscExec.StdOut.ReadAll
    log.WriteLine("dfx seticon " & output)
    If output <> "Success" Then
        log.Close
        Exit Function
    End If

    Set wscExec = wscShell.Exec("DfxSetupDrv.exe defaultbuffersize")
    output = wscExec.StdOut.ReadAll
    log.WriteLine("dfx buffersize " & output)
    If output = "Failed" Then
        log.Close
        Exit Function
    Else
        wscShell.RegWrite "HKLM\Software\DFX\" & VENDOR_CODE & "\devices\default_buffer_size", output, "REG_SZ"
    End If

    wscShell.Exec("powercfg -REQUESTSOVERRIDE DRIVER ""DFX Audio Enhancer"" SYSTEM")

    InstallDFXDriver = True
    log.Close
End Function

If WScript.Arguments.Count = 0 Then
    WScript.Quit(0)
End If

path = WScript.Arguments.Item(0)

If Len(path) = 0 Then
    WScript.Quit(0)
End If

Dim dfxInstalled
dfxInstalled = False
Set wmi = GetObject("winmgmts:\\.\root\cimv2")
Set drivers = wmi.ExecQuery("SELECT * FROM Win32_PnPSignedDriver")
For Each driver In drivers
    If driver.HardWareID = "*DFX12" And driver.DriverVersion = "12.0.0.0" Then
       dfxInstalled = True
       WScript.Quit(0)
    End If
Next

If Not dfxInstalled Then
    ret = InstallDFXDriver(path)
    If ret=True Then
       WScript.Quit(0)
    Else
       WScript.Quit(1)
    End If
End If

