
# xv6 Pipe-Based Concurrent Prime Sieve - Design Notes & Diagrams

<style>
  .gen   { color:#2563eb; font-weight:600; }      /* blue */
  .stage { color:#16a34a; font-weight:600; }      /* green */
  .print { color:#7c3aed; font-weight:600; }      /* violet */
  .filt  { color:#d97706; font-weight:600; }      /* amber/orange */
  .fd    { color:#6b7280; font-weight:600; }      /* gray */
  .eof   { color:#dc2626; font-weight:600; }      /* red */
  .pipe  { color:#0ea5e9; font-weight:600; }      /* cyan */
  pre.diagram { 
    background:#0b1020; 
    color:#e5e7eb; 
    padding:12px; 
    border-radius:10px; 
    overflow:auto; 
    line-height:1.35; 
    font-size:0.95rem;
  }
  code.k { color:#f59e0b; font-weight:600; }      /* keyword-ish */
</style>

## 1) Detailed Flow with Tiny Example (2…15) — <span class="print">print-as-discovered</span>

<pre class="diagram">
<span class="gen">Gen</span> ──► 2 3 4 5 6 7 8 9 10 11 12 13 14 15

<span class="stage">Stage(2):</span>
  read first = 2 → <span class="print">print "prime 2"</span>
  <span class="filt">forward</span>: 3 5 7 9 11 13 15  <span class="fd">(drop multiples of 2)</span>

<span class="stage">Stage(3):</span>
  read first = 3 → <span class="print">print "prime 3"</span>
  <span class="filt">forward</span>: 5 7 11 13         <span class="fd">(drop multiples of 3)</span>

<span class="stage">Stage(5):</span>
  read first = 5 → <span class="print">print "prime 5"</span>
  <span class="filt">forward</span>: 7 11 13           <span class="fd">(drop multiples of 5)</span>

<span class="stage">Stage(7):</span>
  read first = 7 → <span class="print">print "prime 7"</span>
  <span class="filt">forward</span>: 11 13

<span class="stage">Stage(11):</span>
  read first = 11 → <span class="print">print "prime 11"</span>
  <span class="filt">forward</span>: 13

<span class="stage">Stage(13):</span>
  read first = 13 → <span class="print">print "prime 13"</span>
  <span class="filt">forward</span>: (empty)

<span class="fd">EOF cascades; stages exit in reverse order.</span>
</pre>

## 2) Stage Creation & FD Ownership (Process indices start at 0)

**Mapping (indices → roles):**
- **P0**: Generator (writes 2..280)
- **P1**: Stage[1] — discovers prime 2
- **P2**: Stage[2] — discovers prime 3
- **P3**: Stage[3] — discovers prime 5
- … and so on

### 2.1 Generic stage creation (from Pi to P(i+1))

**Before <code class="k">fork()</code> (in Pi):**
<pre class="diagram">
owns:  <span class="fd">Li.r</span>          <span class="fd">Ri.r</span>  <span class="fd">Ri.w</span>
       (left/read)   (new) (new)
</pre>

**After <code class="k">fork()</code>:**
<pre class="diagram">
Parent  Pi (<span class="filt">filter</span> for p_i):        Child  P(i+1) (next stage):
  keep:  <span class="fd">Li.r</span>, <span class="fd">Ri.w</span>                    keep:  <span class="fd">Ri.r</span>
  close: <span class="fd">Ri.r</span>                          close: <span class="fd">Li.r</span>, <span class="fd">Ri.w</span>
  do:    read <span class="fd">Li.r</span>; if x%p_i!=0 → <span class="fd">Ri.w</span>  do:   <span class="k">primes</span>(<span class="fd">Ri.r</span>)  // tail call
</pre>

On upstream EOF in Pi:
<pre class="diagram">
<span class="k">close</span>(<span class="fd">Li.r</span>); <span class="k">close</span>(<span class="fd">Ri.w</span>)  → <span class="eof">EOF to P(i+1)</span>
<span class="k">wait(0)</span>; <span class="k">exit(0)</span>
</pre>

Where <code class="fd">Li</code> is the pipe <strong>feeding</strong> Stage[i] (for i=1, <code class="fd">L1 = IN</code>), and <code class="fd">Ri</code> is the pipe Stage[i] <strong>creates</strong> for Stage[i+1].

### 2.2 First two transitions

**P0 → P1 (Stage[1], p1=2):**
<pre class="diagram">
P0 creates <span class="pipe">IN</span>; <span class="k">fork</span>
  P0: writes 2..280 to <span class="pipe">IN.w</span>, then <span class="k">close</span>(<span class="pipe">IN.w</span>)
  P1: <span class="k">primes</span>(<span class="pipe">IN.r</span>)  // Stage[1]
      read first=2 → <span class="print">print 2</span>
      build <span class="pipe">R1</span>; <span class="k">fork</span>
        P1(parent): <span class="filt">filter</span> <span class="pipe">IN.r</span> → <span class="pipe">R1.w</span> (drop %2==0), then <span class="k">close</span>(<span class="pipe">IN.r</span>,<span class="pipe">R1.w</span>), <span class="k">wait</span>, <span class="k">exit</span>
        P2(child) : <span class="k">primes</span>(<span class="pipe">R1.r</span>) // Stage[2]
</pre>

**P1 → P2 (Stage[2], p2=3):**
<pre class="diagram">
P2 reads first=3 → <span class="print">print 3</span>
build <span class="pipe">R2</span>; <span class="k">fork</span>
  P2(parent): <span class="filt">filter</span> <span class="pipe">R1.r</span> → <span class="pipe">R2.w</span> (drop %3==0), then <span class="k">close</span>(<span class="pipe">R1.r</span>,<span class="pipe">R2.w</span>), <span class="k">wait</span>, <span class="k">exit</span>
  P3(child) : <span class="k">primes</span>(<span class="pipe">R2.r</span>) // Stage[3]
</pre>

### 2.3 EOF cascade (left → right)
<pre class="diagram">
P0 closes <span class="pipe">IN.w</span>  ─►  P1 sees <span class="eof">EOF</span> on <span class="fd">L1.r</span>
P1 closes <span class="pipe">R1.w</span>  ─►  P2 sees <span class="eof">EOF</span> on <span class="fd">L2.r</span> (= <span class="pipe">R1.r</span>)
P2 closes <span class="pipe">R2.w</span>  ─►  P3 sees <span class="eof">EOF</span> on <span class="fd">L3.r</span> (= <span class="pipe">R2.r</span>)
...
</pre>

Each stage’s <strong>parent</strong> (the filter) closes its right write-end; once <em>all</em> writers for a pipe are closed, the next stage’s <code class="k">read()</code> returns 0 (<span class="eof">EOF</span>), and termination unwinds cleanly.

---

## 3) FD Ownership Checklists

**Parent of Stage[i] (filter):**
- Close <code class="fd">Ri.r</code> immediately (you only write to the right pipe).
- Keep <code class="fd">Li.r</code> (read) and <code class="fd">Ri.w</code> (write) during filtering.
- After the loop: <code class="k">close</code>(<span class="fd">Li.r</span>); <code class="k">close</code>(<span class="fd">Ri.w</span>); <code class="k">wait(0)</code>; <code class="k">exit(0)</code>.

**Child (next stage):**
- Close <code class="fd">Ri.w</code> and <code class="fd">Li.r</code> immediately.
- Tail-call <code class="k">primes</code>(<span class="fd">Ri.r</span>) (never returns).

---
