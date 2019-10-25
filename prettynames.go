package pretty

import (
	"fmt"
	"io/ioutil"
	"os"
	"strings"
)

type Renamer struct {
	Dryrun bool
	Hidden bool
	Quiet  bool
}

func makepath(prev, next string) string {
	buf := strings.Builder{}
	buf.WriteString(prev)
	buf.WriteString(next)
	return buf.String()
}

func ishidden(s string) bool {
	if len(s) > 0 && s[0] == '.' {
		return true
	}
	return false
}

func (r *Renamer) Replace(dir string) error {
	finfo, err := ioutil.ReadDir(dir)
	if err != nil {
		return err
	}
	for _, f := range finfo {
		if !r.Hidden && ishidden(f.Name()) {
			continue
		}
		if f.IsDir() {
			err = r.Replace(makepath(dir, f.Name()))
			if err != nil {
				return err
			}
		}
	}
	for _, f := range finfo {
		if !r.Hidden && ishidden(f.Name()) {
			continue
		}
		err = r.rename(dir, f.Name())
		if err != nil {
			return err
		}
	}
	return nil
}

func (r *Renamer) rename(dir, fname string) error {
	b := []byte(fname)
	modified := false

	for i := 0; i < len(b); {
		// strip invalid characters
		if b[i] < 32 || (b[i] > 126 && b[i] <= 255) {
			modified = true
			b = append(b[:i], b[i+1:]...)
			continue
		}

		// replace invalid characters
		if b[i] == ' ' || b[i] == '|' || b[i] == '&' || b[i] == ';' || b[i] == '(' ||
			b[i] == ')' || b[i] == '<' || b[i] == '>' || b[i] == '!' || b[i] == '[' ||
			b[i] == ']' || b[i] == '{' || b[i] == '}' || b[i] == ':' || b[i] == '?' ||
			b[i] == '\'' ||
			b[i] == '=' || b[i] == '*' || b[i] == '/' || b[i] == '"' || b[i] == '\\' {
			modified = true
			b[i] = '_'
		}

		i++
	}

	if modified {
		newname := string(b)
		newpath := makepath(dir, newname)
		oldpath := makepath(dir, fname)
		if r.Dryrun {
			if !r.Quiet {
				fmt.Printf("[dryrun] %s → %s\n", oldpath, newpath)
			}
		} else {
			err := os.Rename(oldpath, newpath)
			if err != nil {
				return err
			}
			if !r.Quiet {
				fmt.Printf("%s → %s\n", oldpath, newpath)
			}
		}
	}

	return nil
}
