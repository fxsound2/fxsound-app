//go:build windows

package fxsound

import (
	"os"
	"os/exec"
	"path/filepath"

	"golang.org/x/sys/windows/registry"
)

// Paths holds the resolved locations of the FxSound executables this
// server drives.
type Paths struct {
	FxSoundExe string
	FxDiagExe  string
}

// Locate resolves the install locations of FxSound.exe and fxdiag.exe.
//
// Resolution order: the installer's default Program Files location, the
// Windows uninstall registry (in case of a custom install directory), then
// PATH.
func Locate() (*Paths, error) {
	p := &Paths{}

	for _, dir := range candidateDirs() {
		if p.FxSoundExe == "" {
			if exe := filepath.Join(dir, "FxSound.exe"); fileExists(exe) {
				p.FxSoundExe = exe
			}
		}
		if p.FxDiagExe == "" {
			if exe := filepath.Join(dir, "fxdiag.exe"); fileExists(exe) {
				p.FxDiagExe = exe
			}
		}
		if p.FxSoundExe != "" && p.FxDiagExe != "" {
			break
		}
	}

	if p.FxSoundExe == "" {
		if exe, err := exec.LookPath("FxSound.exe"); err == nil {
			p.FxSoundExe = exe
		}
	}
	if p.FxDiagExe == "" {
		if exe, err := exec.LookPath("fxdiag.exe"); err == nil {
			p.FxDiagExe = exe
		}
	}

	return p, validate(p)
}

func validate(p *Paths) error {
	var missing []string
	if p.FxSoundExe == "" {
		missing = append(missing, "FxSound.exe")
	}
	if p.FxDiagExe == "" {
		missing = append(missing, "fxdiag.exe")
	}
	if len(missing) > 0 {
		return newError(ErrKindAppNotFound, nil, "could not locate %v under Program Files, the Windows uninstall registry, or PATH", missing)
	}
	return nil
}

func fileExists(path string) bool {
	info, err := os.Stat(path)
	return err == nil && !info.IsDir()
}

// candidateDirs returns install directories to probe, in priority order:
// the installer's default Program Files location(s) (see
// Installer/fxsound.aip: Manufacturer="FxSound LLC", ProductName="FxSound"),
// then any directory reported by the Windows uninstall registry for
// FxSound, which covers a custom install directory chosen during setup.
func candidateDirs() []string {
	var dirs []string
	for _, envVar := range []string{"ProgramFiles", "ProgramFiles(x86)"} {
		if base := os.Getenv(envVar); base != "" {
			dirs = append(dirs, filepath.Join(base, "FxSound LLC", "FxSound"))
		}
	}
	dirs = append(dirs, registryInstallDirs()...)
	return dirs
}

// registryInstallDirs scans the standard Windows uninstall registry keys
// for an entry whose DisplayName is FxSound, returning its InstallLocation
// if found.
func registryInstallDirs() []string {
	var dirs []string
	roots := []struct {
		key  registry.Key
		path string
	}{
		{registry.LOCAL_MACHINE, `SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall`},
		{registry.LOCAL_MACHINE, `SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall`},
		{registry.CURRENT_USER, `SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall`},
	}
	for _, root := range roots {
		k, err := registry.OpenKey(root.key, root.path, registry.ENUMERATE_SUB_KEYS)
		if err != nil {
			continue
		}
		names, err := k.ReadSubKeyNames(-1)
		k.Close()
		if err != nil {
			continue
		}
		for _, name := range names {
			sk, err := registry.OpenKey(root.key, root.path+`\`+name, registry.QUERY_VALUE)
			if err != nil {
				continue
			}
			displayName, _, err := sk.GetStringValue("DisplayName")
			if err == nil && displayName == "FxSound" {
				if loc, _, err := sk.GetStringValue("InstallLocation"); err == nil && loc != "" {
					dirs = append(dirs, loc)
				}
			}
			sk.Close()
		}
	}
	return dirs
}
