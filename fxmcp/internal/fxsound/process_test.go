package fxsound

import (
	"bytes"
	"context"
	"sync"
	"testing"
	"time"
)

// TestSpawnsAreSerialized confirms concurrent Run calls never overlap in
// time -- the property that keeps overlapping tool calls from racing on
// status.json (one call's mtime-poll picking up a write caused by a
// different, concurrent call).
func TestSpawnsAreSerialized(t *testing.T) {
	const holdTime = 20 * time.Millisecond

	orig := runProcess
	var mu sync.Mutex
	var intervals [][2]time.Time
	runProcess = func(_ context.Context, _ string, _ []string, _, _ *bytes.Buffer) error {
		start := time.Now()
		time.Sleep(holdTime)
		end := time.Now()
		mu.Lock()
		intervals = append(intervals, [2]time.Time{start, end})
		mu.Unlock()
		return nil
	}
	t.Cleanup(func() { runProcess = orig })

	paths := &Paths{FxSoundExe: `C:\fake\FxSound.exe`}

	const n = 5
	var wg sync.WaitGroup
	wg.Add(n)
	for i := 0; i < n; i++ {
		go func() {
			defer wg.Done()
			_, _ = Run(context.Background(), time.Second, paths.FxSoundExe, "--status")
		}()
	}
	wg.Wait()

	if len(intervals) != n {
		t.Fatalf("expected %d spawns, got %d", n, len(intervals))
	}
	for i := 0; i < len(intervals); i++ {
		for j := i + 1; j < len(intervals); j++ {
			a, b := intervals[i], intervals[j]
			if a[0].Before(b[1]) && b[0].Before(a[1]) {
				t.Errorf("spawn intervals overlapped: [%v,%v] and [%v,%v]", a[0], a[1], b[0], b[1])
			}
		}
	}
}
