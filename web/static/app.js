(() => {
  'use strict';

  // ── Helpers ──

  const SUBS = '\u2080\u2081\u2082\u2083\u2084\u2085\u2086\u2087\u2088\u2089';
  const sub = n => String(n).split('').map(d => SUBS[+d]).join('');
  const vn = i => `x${sub(i + 1)}`;
  const $ = id => document.getElementById(id);
  const esc = s => {
    const d = document.createElement('div');
    d.textContent = s;
    return d.innerHTML;
  };

  // ── State ──

  let numVars = 2;
  let cons = [{ coefficients: ['', ''], sense: '<=', rhs: '' }];
  let lastResult = null;

  // Task queue state
  let tasks = [];          // { id, num, state, result, error, submittedAt, label }
  let taskCounter = 0;
  let selectedTaskId = null;
  let _elapsedTimer = null;

  // ── Examples ──

  const EXAMPLES = [
    {
      name: 'Production Planning',
      numVars: 2,
      objective: { sense: 'maximize', coefficients: ['30', '20'] },
      constraints: [
        { coefficients: ['2', '1'], sense: '<=', rhs: '100' },
        { coefficients: ['1', '1'], sense: '<=', rhs: '80' },
      ],
      basis: 'big-m',
    },
    {
      name: 'Mixed Constraints',
      numVars: 2,
      objective: { sense: 'maximize', coefficients: ['5', '4'] },
      constraints: [
        { coefficients: ['6', '4'], sense: '<=', rhs: '24' },
        { coefficients: ['1', '2'], sense: '<=', rhs: '6' },
        { coefficients: ['1', '-1'], sense: '>=', rhs: '1' },
      ],
      basis: 'big-m',
    },
    {
      name: 'Minimization',
      numVars: 2,
      objective: { sense: 'minimize', coefficients: ['2', '3'] },
      constraints: [
        { coefficients: ['1', '2'], sense: '>=', rhs: '4' },
        { coefficients: ['2', '1'], sense: '>=', rhs: '6' },
      ],
      basis: 'artificial',
    },
    {
      name: '3-Variable Problem',
      numVars: 3,
      objective: { sense: 'minimize', coefficients: ['2', '3', '5'] },
      constraints: [
        { coefficients: ['1', '0', '1'], sense: '>=', rhs: '4' },
        { coefficients: ['0', '1', '1'], sense: '>=', rhs: '3' },
        { coefficients: ['1', '1', '0'], sense: '<=', rhs: '8' },
      ],
      basis: 'big-m',
    },
    {
      name: 'Large: Resource Allocation (5-var)',
      numVars: 5,
      objective: { sense: 'maximize', coefficients: ['10', '6', '4', '8', '5'] },
      constraints: [
        { coefficients: ['1',  '1',  '1',  '1',  '1' ], sense: '<=', rhs: '100' },
        { coefficients: ['10', '4',  '5',  '6',  '3' ], sense: '<=', rhs: '600' },
        { coefficients: ['2',  '2',  '6',  '4',  '8' ], sense: '<=', rhs: '400' },
        { coefficients: ['3',  '5',  '2',  '1',  '4' ], sense: '<=', rhs: '300' },
        { coefficients: ['1',  '3',  '4',  '2',  '1' ], sense: '<=', rhs: '250' },
        { coefficients: ['5',  '2',  '1',  '3',  '6' ], sense: '<=', rhs: '500' },
      ],
      basis: 'big-m',
    },
    {
      name: 'Large: Supply Chain (6-var, mixed)',
      numVars: 6,
      objective: { sense: 'minimize', coefficients: ['3', '5', '2', '7', '4', '6'] },
      constraints: [
        { coefficients: ['1',  '0',  '1',  '0',  '1',  '0' ], sense: '>=', rhs: '80'  },
        { coefficients: ['0',  '1',  '0',  '1',  '0',  '1' ], sense: '>=', rhs: '70'  },
        { coefficients: ['1',  '1',  '0',  '0',  '1',  '1' ], sense: '>=', rhs: '120' },
        { coefficients: ['2',  '3',  '1',  '2',  '1',  '3' ], sense: '<=', rhs: '500' },
        { coefficients: ['1',  '2',  '3',  '1',  '2',  '1' ], sense: '<=', rhs: '450' },
        { coefficients: ['3',  '1',  '2',  '3',  '1',  '2' ], sense: '<=', rhs: '480' },
        { coefficients: ['1',  '1',  '1',  '1',  '0',  '0' ], sense: '<=', rhs: '200' },
        { coefficients: ['0',  '0',  '1',  '1',  '1',  '1' ], sense: '<=', rhs: '180' },
      ],
      basis: 'artificial',
    },
  ];

  // ── Rendering: Form ──

  function renderObjective() {
    const el = $('obj-coeffs');
    const old = Array.from(el.querySelectorAll('.coeff-input')).map(i => i.value);
    el.innerHTML = '';
    for (let i = 0; i < numVars; i++) {
      if (i > 0) el.insertAdjacentHTML('beforeend', '<span class="plus">+</span>');
      el.insertAdjacentHTML('beforeend',
        `<span class="term"><input type="text" class="coeff-input" placeholder="0" value="${old[i] || ''}"><span class="var-label">${vn(i)}</span></span>`);
    }
    el.querySelectorAll('.coeff-input').forEach(inp => inp.addEventListener('input', updatePreview));
  }

  function renderConstraints() {
    const el = $('constraints');
    el.innerHTML = '';
    cons.forEach((c, ci) => {
      let coeffs = '';
      for (let i = 0; i < numVars; i++) {
        if (i > 0) coeffs += '<span class="plus">+</span>';
        coeffs += `<span class="term"><input type="text" class="coeff-input" placeholder="0" value="${c.coefficients[i] || ''}"><span class="var-label">${vn(i)}</span></span>`;
      }
      const sensOpts = [['<=', '\u2264'], ['=', '='], ['>=', '\u2265']];
      const sensHtml = sensOpts.map(([v, lbl]) =>
        `<option value="${v}"${c.sense === v ? ' selected' : ''}>${lbl}</option>`).join('');

      el.insertAdjacentHTML('beforeend',
        `<div class="constraint-row" data-ci="${ci}">
          <div class="coeff-row">${coeffs}</div>
          <select class="select con-sense">${sensHtml}</select>
          <input type="text" class="coeff-input rhs-input" placeholder="0" value="${c.rhs || ''}">
          <button class="btn btn-danger remove-con" title="Remove">&times;</button>
        </div>`);
    });

    el.querySelectorAll('.coeff-input, .con-sense').forEach(inp => {
      inp.addEventListener('input', () => { syncCons(); updatePreview(); });
      inp.addEventListener('change', () => { syncCons(); updatePreview(); });
    });
    el.querySelectorAll('.remove-con').forEach(btn => {
      btn.addEventListener('click', () => {
        if (cons.length <= 1) return;
        cons.splice(+btn.closest('.constraint-row').dataset.ci, 1);
        renderConstraints();
        updatePreview();
      });
    });
  }

  function syncCons() {
    document.querySelectorAll('.constraint-row').forEach((row, ci) => {
      if (!cons[ci]) return;
      cons[ci].coefficients = Array.from(row.querySelectorAll('.coeff-row .coeff-input')).map(i => i.value);
      cons[ci].sense = row.querySelector('.con-sense').value;
      cons[ci].rhs = row.querySelector('.rhs-input').value;
    });
  }

  // ── Rendering: Preview ──

  function updatePreview() {
    syncCons();
    const sense = $('obj-sense').value;
    const objC = Array.from($('obj-coeffs').querySelectorAll('.coeff-input')).map(i => i.value);

    // Don't show anything if the form is completely empty
    const hasAnyValue = objC.some(v => v.trim() !== '')
      || cons.some(c => c.coefficients.some(v => v.trim() !== '') || c.rhs.trim() !== '');
    if (!hasAnyValue) {
      $('preview').innerHTML = '<span style="color:var(--text-muted);font-style:italic">Fill in the form above to see a preview</span>';
      return;
    }

    let html = `<span class="kw">${cap(sense)}:</span>  ${fmtExpr(objC)}\n\n`;
    html += '<span class="kw">Subject to:</span>\n';
    cons.forEach(c => {
      const sym = { '<=': '\u2264', '=': '=', '>=': '\u2265' }[c.sense] || c.sense;
      html += `  ${fmtExpr(c.coefficients)}  ${sym}  ${c.rhs || '0'}\n`;
    });
    html += `\n  ${Array.from({ length: numVars }, (_, i) => vn(i)).join(', ')} \u2265 0`;
    $('preview').innerHTML = html;
  }

  function fmtExpr(coefficients) {
    const terms = [];
    coefficients.forEach((raw, i) => {
      const c = (raw || '').trim();
      if (!c || c === '0') return;
      const neg = c.startsWith('-');
      const abs = neg ? c.slice(1) : c;
      const v = vn(i);
      const t = abs === '1' ? v : `${abs}${v}`;
      if (terms.length === 0) {
        terms.push(neg ? `\u2212${t}` : t);
      } else {
        terms.push(neg ? ` \u2212 ${t}` : ` + ${t}`);
      }
    });
    return terms.length ? terms.join('') : '0';
  }

  function cap(s) { return s[0].toUpperCase() + s.slice(1); }

  // ── Rendering: Solution ──

  function showSolution(data) {
    const card = $('solution-card');
    const body = $('solution-body');
    card.classList.remove('hidden');

    if (data.status === 'error') {
      body.innerHTML = `<div class="error-banner">${esc(data.error)}</div>`;
      return;
    }

    const sol = data.solution;
    const labels = { optimal: 'Optimal', infeasible: 'Infeasible', unbounded: 'Unbounded' };
    let h = `<div class="solution-status status-${sol.status}">${labels[sol.status] || sol.status}</div>`;

    if (sol.status === 'optimal') {
      h += '<div class="solution-details">';
      h += `<div class="solution-row"><span class="label">Objective Value</span><span class="value">${sol.objectiveValue}</span></div>`;
      (sol.variableValues || []).forEach((v, i) => {
        h += `<div class="solution-row"><span class="label">${vn(i)}</span><span class="value">${v}</span></div>`;
      });
      h += '</div>';
    }
    body.innerHTML = h;
  }

  // ── Rendering: Iterations ──

  function showIterations(data) {
    const card = $('iterations-card');
    const body = $('iterations-body');

    if (data.status === 'error' || !data.iterations || !data.iterations.length) {
      card.classList.add('hidden');
      return;
    }
    card.classList.remove('hidden');

    const bf = data.basisFinding.result;
    const colHeaders = buildColHeaders(bf);

    // Detect phases using the phase field
    const phases = new Set(data.iterations.map(it => it.phase));
    const multiPhase = phases.size > 1;

    let html = '';
    let currentPhase = -1;

    data.iterations.forEach((it, idx) => {
      if (multiPhase && it.phase !== currentPhase) {
        currentPhase = it.phase;
        html += `<div class="phase-separator">Phase ${currentPhase}</div>`;
      }

      const isFirst = idx === 0;
      const isLast = idx === data.iterations.length - 1;

      html += `<div class="iteration${isFirst || isLast ? ' open' : ''}">
        <div class="iteration-header" onclick="this.parentElement.classList.toggle('open')">
          <span>Iteration ${it.iterationNumber}</span>
          <span>
            <span class="meta">z = ${it.objectiveValue}</span>
            <span class="chevron"> \u25B6</span>
          </span>
        </div>
        <div class="iteration-body">${renderTableau(it, colHeaders)}</div>
      </div>`;
    });

    body.innerHTML = html;
  }

  function buildColHeaders(bf) {
    const total = bf.augmented.constraintMatrix.cols;
    const headers = new Array(total);
    for (let i = 0; i < bf.originalVariableCount; i++) headers[i] = vn(i);
    let si = 1, ei = 1, ai = 1;
    (bf.slackColumns || []).forEach(c => { headers[c] = `s${sub(si++)}`; });
    (bf.surplusColumns || []).forEach(c => { headers[c] = `e${sub(ei++)}`; });
    (bf.artificialColumns || []).forEach(c => { headers[c] = `a${sub(ai++)}`; });
    return headers;
  }

  function renderTableau(it, allColHeaders) {
    const { tableau, basisVariables, pivotRow, pivotColumn, ratios } = it;
    const tableCols = tableau.cols;

    // Separate constraint rows from the objective row (last row)
    const constraintRows = tableau.data.slice(0, -1);
    const objectiveRow = tableau.data[tableau.data.length - 1];

    // Build headers for this specific tableau (may differ in phase 2)
    // tableCols includes RHS column; allColHeaders does not include RHS
    // Objective row adds +1 to tableau.rows but not to cols
    let varHeaders;
    if (tableCols === allColHeaders.length + 1) {
      varHeaders = [...allColHeaders];
    } else {
      varHeaders = [];
      for (let i = 0; i < tableCols - 1; i++) {
        varHeaders.push(allColHeaders[i] || `c${sub(i + 1)}`);
      }
    }

    const hasRatios = ratios && ratios.some(r => r !== null);

    let h = '<div class="tableau-wrapper"><table class="tableau"><thead><tr>';
    h += '<th>I</th><th>Basis</th>';
    varHeaders.forEach(hdr => { h += `<th>${hdr}</th>`; });
    h += `<th>\u0056\u0305<sub>b</sub></th>`;
    if (hasRatios) h += '<th>Ratio</th>';
    h += '</tr></thead><tbody>';

    // Constraint rows with iteration number spanning all rows
    const totalRows = constraintRows.length + 1; // +1 for objective row
    constraintRows.forEach((row, ri) => {
      const bv = basisVariables[ri];
      const basisLabel = bv !== undefined ? (allColHeaders[bv] || `c${sub(bv + 1)}`) : '?';
      h += '<tr>';
      if (ri === 0) {
        h += `<td class="iter-cell" rowspan="${totalRows}">${it.iterationNumber}</td>`;
      }
      h += `<td class="basis-label">${basisLabel}</td>`;
      row.forEach((cell, ci) => {
        const isPivot = pivotRow !== null && pivotColumn !== null && ri === pivotRow && ci === pivotColumn;
        h += `<td${isPivot ? ' class="pivot"' : ''}>${cell}</td>`;
      });
      if (hasRatios) {
        const r = ratios[ri];
        h += `<td class="ratio-cell">${r !== null ? r : '\u2014'}</td>`;
      }
      h += '</tr>';
    });

    // Objective row (z-row)
    h += '<tr class="z-row">';
    h += '<td class="basis-label">z</td>';
    objectiveRow.forEach((cell, ci) => {
      h += `<td>${cell}</td>`;
    });
    if (hasRatios) h += '<td></td>';
    h += '</tr>';

    h += '</tbody></table></div>';
    return h;
  }

  // ── Queue status indicator ──

  function updateQueueIndicator(depth) {
    const dot   = $('queue-dot');
    const label = $('queue-label');
    if (depth < 0) {
      dot.className = 'queue-dot dot-unknown';
      label.textContent = 'Queue: —';
    } else if (depth === 0) {
      dot.className = 'queue-dot dot-idle';
      label.textContent = 'Queue: idle';
    } else {
      dot.className = 'queue-dot dot-busy';
      label.textContent = `Queue: ${depth}`;
    }
  }

  function startQueuePolling() {
    const poll = async () => {
      try {
        const r = await fetch('/api/status');
        const d = await r.json();
        updateQueueIndicator(d.queue_depth ?? -1);
      } catch {
        updateQueueIndicator(-1);
      }
    };
    poll();
    setInterval(poll, 5000);
  }

  function updateWorkersIndicator(count) {
    const dot   = $('workers-dot');
    const label = $('workers-label');
    if (count < 0) {
      dot.className     = 'status-dot wdot-unknown';
      label.textContent = 'Workers: \u2014';
    } else if (count === 0) {
      dot.className     = 'status-dot wdot-idle';
      label.textContent = 'Workers: 0';
    } else {
      dot.className     = 'status-dot wdot-active';
      label.textContent = `Workers: ${count}`;
    }
  }

  function startWorkersPolling() {
    const poll = async () => {
      try {
        const r = await fetch('/api/workers');
        const d = await r.json();
        updateWorkersIndicator(d.count ?? -1);
      } catch {
        updateWorkersIndicator(-1);
      }
    };
    poll();
    setInterval(poll, 10000);
  }

  // ── Task list ──

  function formatElapsed(ms) {
    const s = Math.floor(ms / 1000);
    if (s < 60) return `${s}s`;
    return `${Math.floor(s / 60)}m ${s % 60}s`;
  }

  function buildTaskLabel(payload) {
    const sense = payload.objective.sense === 'maximize' ? 'max' : 'min';
    const terms = payload.objective.coefficients
      .map((c, i) => (c === '0' || c === '') ? null : `${c}${vn(i)}`)
      .filter(Boolean);
    const expr = terms.join('+') || '0';
    const label = `${sense} ${expr}`;
    return label.length > 38 ? label.slice(0, 35) + '…' : label;
  }

  function renderInlineResult(t) {
    const p   = t.payload;
    const res = t.result;
    let html  = '<div class="task-inline-result">';

    // ── Input problem ──────────────────────────────────────────────────────
    if (p) {
      const sense = cap(p.objective.sense);
      const objExpr = p.objective.coefficients
        .map((c, i) => (c === '0' || c === '') ? null : `${c}${vn(i)}`)
        .filter(Boolean).join(' + ') || '0';
      let preview = `${sense}:  ${objExpr}\n\nSubject to:\n`;
      p.constraints.forEach(con => {
        const sym  = { '<=': '\u2264', '=': '=', '>=': '\u2265' }[con.sense] || con.sense;
        const expr = con.coefficients
          .map((c, i) => (c === '0' || c === '') ? null : `${c}${vn(i)}`)
          .filter(Boolean).join(' + ') || '0';
        preview += `  ${expr}  ${sym}  ${con.rhs}\n`;
      });
      const n = p.objective.coefficients.length;
      preview += `\n  ${Array.from({ length: n }, (_, i) => vn(i)).join(', ')} \u2265 0`;
      html += `<div class="task-inline-section">
        <span class="task-inline-label">Input</span>
        <pre class="task-inline-preview">${esc(preview)}</pre>
      </div>`;
    }

    // ── Solution ───────────────────────────────────────────────────────────
    if (res) {
      if (res.status === 'error') {
        html += `<div class="task-inline-section">
          <span class="task-inline-label">Error</span>
          <div class="error-banner">${esc(res.error)}</div>
        </div>`;
      } else if (res.solution) {
        const sol = res.solution;
        const sc  = { optimal: 'status-optimal', infeasible: 'status-infeasible', unbounded: 'status-unbounded' }[sol.status] || '';
        const sl  = { optimal: 'Optimal', infeasible: 'Infeasible', unbounded: 'Unbounded' }[sol.status] || sol.status;
        let solHtml = `<div class="solution-status ${sc}">${sl}</div>`;
        if (sol.status === 'optimal') {
          solHtml += '<div class="solution-details">';
          solHtml += `<div class="solution-row"><span class="label">Objective Value</span><span class="value">${sol.objectiveValue}</span></div>`;
          (sol.variableValues || []).forEach((v, i) => {
            solHtml += `<div class="solution-row"><span class="label">${vn(i)}</span><span class="value">${v}</span></div>`;
          });
          solHtml += '</div>';
        }
        html += `<div class="task-inline-section">
          <span class="task-inline-label">Solution</span>
          ${solHtml}
        </div>`;
      }
    }

    html += '</div>';
    return html;
  }

  function renderTaskList() {
    const card = $('tasks-card');
    const list = $('tasks-list');
    if (!tasks.length) {
      card.classList.add('hidden');
      return;
    }
    card.classList.remove('hidden');

    const STATE = {
      PENDING:  { dot: 'dot-queued',  text: 'Queued'  },
      STARTED:  { dot: 'dot-running', text: 'Running' },
      PROGRESS: { dot: 'dot-running', text: 'Solving' },
      SUCCESS:  { dot: 'dot-success', text: 'Done'    },
      FAILURE:  { dot: 'dot-failure', text: 'Failed'  },
    };

    list.innerHTML = tasks.map(t => {
      const info    = STATE[t.state] || STATE.PENDING;
      const elapsed = formatElapsed(Date.now() - t.submittedAt);
      const isSelected = t.id === selectedTaskId;
      const canView    = t.state === 'SUCCESS';
      const isActive   = t.state === 'PENDING' || t.state === 'STARTED' || t.state === 'PROGRESS';

      let sub = info.text;
      if (t.state === 'PENDING' && t.position != null)
        sub += ` \u00b7 #${t.position}\u200a/\u200a${t.total} in queue`;
      if ((t.state === 'PROGRESS' || t.state === 'STARTED') && t.meta?.step)
        sub += ` \u00b7 ${t.meta.step}`;
      sub += ` \u00b7 ${elapsed}`;

      const pbClass = t.state === 'SUCCESS' ? 'pb-done'
                    : t.state === 'FAILURE' ? 'pb-failed'
                    : isActive              ? 'pb-running'
                    :                         'pb-pending';

      const inlineResult = (isSelected && t.state === 'SUCCESS') ? renderInlineResult(t) : '';

      return `<div class="task-item${isSelected ? ' selected' : ''}${canView ? ' clickable' : ''}" data-id="${t.id}">
        <div class="task-item-main">
          <div class="task-item-left">
            <span class="task-dot ${info.dot}${isActive ? ' pulsing' : ''}"></span>
            <div class="task-item-info">
              <div class="task-item-title">#${t.num} &mdash; ${esc(t.label)}</div>
              <div class="task-item-sub">${sub}</div>
            </div>
          </div>
          <div class="task-item-actions">
            ${isActive ? `<span class="task-spinner"></span>` : ''}
            ${canView ? `<span class="task-chevron${isSelected ? ' open' : ''}" aria-hidden="true">&#8250;</span>` : ''}
            <button class="btn btn-sm btn-icon task-dismiss" data-id="${t.id}" title="Dismiss">&times;</button>
          </div>
        </div>
        <div class="task-progress">
          <div class="task-progress-bar ${pbClass}"></div>
        </div>
        ${inlineResult}
      </div>`;
    }).join('');
  }

  function startElapsedTimer() {
    if (_elapsedTimer) return;
    _elapsedTimer = setInterval(() => {
      const hasActive = tasks.some(
        t => t.state === 'PENDING' || t.state === 'STARTED' || t.state === 'PROGRESS'
      );
      renderTaskList();
      if (!hasActive) {
        clearInterval(_elapsedTimer);
        _elapsedTimer = null;
      }
    }, 1000);
  }

  function startTaskPolling(task) {
    const poll = async () => {
      try {
        const r = await fetch(`/api/result/${task.id}`);
        const data = await r.json();
        task.state = data.state;

        if (data.state === 'PENDING') {
          task.position = data.position;
          task.total    = data.total;
        }
        if (data.state === 'PROGRESS' || data.state === 'STARTED') {
          task.meta = data.meta || {};
        }

        if (data.state === 'SUCCESS') {
          task.result = data.result;
          renderTaskList();
          if (!selectedTaskId) viewTask(task.id);
          return;
        }
        if (data.state === 'FAILURE') {
          task.error = data.error || 'Worker error';
          renderTaskList();
          return;
        }
        renderTaskList();
        setTimeout(poll, 1500);
      } catch {
        setTimeout(poll, 3000);
      }
    };
    setTimeout(poll, 600);
  }

  function viewTask(taskId) {
    const task = tasks.find(t => t.id === taskId);
    if (!task || !task.result) return;
    selectedTaskId = taskId;
    lastResult = task.result;
    $('solution-card').classList.add('hidden');
    showIterations(task.result);
    renderTaskList();
    const row = document.querySelector(`.task-item[data-id="${taskId}"]`);
    if (row) row.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }

  function dismissTask(taskId) {
    tasks = tasks.filter(t => t.id !== taskId);
    if (selectedTaskId === taskId) {
      selectedTaskId = null;
      lastResult = null;
      $('solution-card').classList.add('hidden');
      $('iterations-card').classList.add('hidden');
    }
    renderTaskList();
  }

  function clearDoneTasks() {
    tasks = tasks.filter(t => t.state === 'PENDING' || t.state === 'STARTED');
    if (selectedTaskId) {
      const still = tasks.find(t => t.id === selectedTaskId);
      if (!still) {
        selectedTaskId = null;
        lastResult = null;
        $('solution-card').classList.add('hidden');
        $('iterations-card').classList.add('hidden');
      }
    }
    renderTaskList();
  }

  // ── Actions ──

  function changeVars(delta) {
    const n = numVars + delta;
    if (n < 1 || n > 20) return;
    numVars = n;
    $('var-count').textContent = numVars;
    cons.forEach(c => {
      while (c.coefficients.length < numVars) c.coefficients.push('');
      c.coefficients.length = numVars;
    });
    renderObjective();
    renderConstraints();
    updatePreview();
  }

  function addConstraint() {
    cons.push({ coefficients: new Array(numVars).fill(''), sense: '<=', rhs: '' });
    renderConstraints();
    updatePreview();
  }

  async function solve() {
    const btn = $('solve-btn');
    btn.disabled = true;
    btn.innerHTML = '<span class="spinner"></span>';

    syncCons();
    const payload = {
      objective: {
        sense: $('obj-sense').value,
        coefficients: Array.from($('obj-coeffs').querySelectorAll('.coeff-input')).map(i => i.value || '0'),
      },
      constraints: cons.map(c => ({
        coefficients: c.coefficients.map(v => v || '0'),
        sense: c.sense,
        rhs: c.rhs || '0',
      })),
      basisMethod: $('basis-method').value,
    };

    try {
      const resp = await fetch('/api/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      });
      const json = await resp.json();
      if (json.error) throw new Error(json.error);
      const task_id = json.task_id;

      taskCounter++;
      const task = {
        id: task_id,
        num: taskCounter,
        state: 'PENDING',
        result: null,
        error: null,
        submittedAt: Date.now(),
        label: buildTaskLabel(payload),
        payload,
      };
      tasks.unshift(task);
      renderTaskList();
      startElapsedTimer();
      startTaskPolling(task);

      $('queue-dot').className = 'queue-dot dot-busy';

    } catch (err) {
      showSolution({ status: 'error', error: err.message || 'Network error' });
      $('solution-card').classList.remove('hidden');
    } finally {
      btn.disabled = false;
      btn.textContent = 'Solve';
    }
  }

  function loadExample(ex) {
    numVars = ex.numVars;
    $('var-count').textContent = numVars;
    $('obj-sense').value = ex.objective.sense;
    $('basis-method').value = ex.basis;
    cons = ex.constraints.map(c => ({ coefficients: [...c.coefficients], sense: c.sense, rhs: c.rhs }));

    renderObjective();
    const objInputs = $('obj-coeffs').querySelectorAll('.coeff-input');
    ex.objective.coefficients.forEach((v, i) => { if (objInputs[i]) objInputs[i].value = v; });
    renderConstraints();
    updatePreview();
    $('solution-card').classList.add('hidden');
    $('iterations-card').classList.add('hidden');
  }

  function renderExamples() {
    const el = $('examples');
    EXAMPLES.forEach(ex => {
      const btn = document.createElement('button');
      btn.className = 'example-btn';
      btn.textContent = ex.name;
      btn.addEventListener('click', () => loadExample(ex));
      el.appendChild(btn);
    });
  }

  // ── Import ──

  async function importFile(file) {
    const fd = new FormData();
    fd.append('file', file);
    try {
      const resp = await fetch('/api/import', { method: 'POST', body: fd });
      const data = await resp.json();
      if (data.error) {
        showSolution({ status: 'error', error: `Import error: ${data.error}` });
        $('solution-card').classList.remove('hidden');
        return;
      }
      loadImported(data);
    } catch (e) {
      showSolution({ status: 'error', error: `Import failed: ${e.message}` });
      $('solution-card').classList.remove('hidden');
    }
  }

  function loadImported(data) {
    numVars = data.numVars;
    $('var-count').textContent = numVars;
    $('obj-sense').value = data.objective.sense;
    $('basis-method').value = data.basisMethod || 'big-m';
    cons = data.constraints.map(c => ({
      coefficients: [...c.coefficients],
      sense: c.sense,
      rhs: c.rhs,
    }));
    renderObjective();
    const objInputs = $('obj-coeffs').querySelectorAll('.coeff-input');
    data.objective.coefficients.forEach((v, i) => { if (objInputs[i]) objInputs[i].value = v; });
    renderConstraints();
    updatePreview();
    $('solution-card').classList.add('hidden');
    $('iterations-card').classList.add('hidden');
    lastResult = null;
  }

  // ── Export ──

  function downloadBlob(content, mime, filename) {
    const blob = new Blob([content], { type: mime });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  function downloadReport(format) {
    if (!lastResult) return;
    if (format === 'html') {
      downloadBlob(generateHtmlReport(), 'text/html', 'limo-report.html');
    } else if (format === 'txt') {
      downloadBlob(generateTxtReport(), 'text/plain', 'limo-report.txt');
    } else {
      downloadBlob(JSON.stringify(lastResult, null, 2), 'application/json', 'limo-report.json');
    }
  }

  function generateHtmlReport() {
    const problemText = $('preview').textContent;
    const solutionHtml = $('solution-body').innerHTML;
    // Open all iterations
    const iterHtml = $('iterations-body').innerHTML
      .replace(/class="iteration(?:\s+open)?"/g, 'class="iteration open"');
    const now = new Date().toLocaleString();

    return `<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8"><title>LIMO Report</title>
<style>
body{font-family:system-ui,-apple-system,sans-serif;max-width:1100px;margin:40px auto;padding:0 24px;color:#1e293b;background:#f1f5f9}
h1{font-size:22px;font-weight:800;color:#2563eb;margin-bottom:4px}
.report-meta{font-size:12px;color:#94a3b8;margin-bottom:32px}
h2{font-size:11px;font-weight:700;text-transform:uppercase;letter-spacing:.6px;color:#64748b;margin:28px 0 10px;padding:10px 16px;background:#fafbfc;border:1px solid #e2e8f0;border-radius:8px 8px 0 0}
pre{font-family:'JetBrains Mono',monospace;font-size:13px;line-height:2;white-space:pre-wrap;background:#fff;padding:16px;border:1px solid #e2e8f0;border-top:none;border-radius:0 0 8px 8px;margin-top:0}
.solution-block{background:#fff;border:1px solid #e2e8f0;border-top:none;border-radius:0 0 8px 8px;padding:16px;margin-top:0}
.solution-status{display:inline-flex;padding:5px 14px;border-radius:20px;font-size:12px;font-weight:700;text-transform:uppercase;letter-spacing:.5px}
.status-optimal{background:#f0fdf4;color:#16a34a;border:1px solid #bbf7d0}
.status-infeasible{background:#fef2f2;color:#dc2626;border:1px solid #fecaca}
.status-unbounded{background:#fffbeb;color:#d97706;border:1px solid #fde68a}
.solution-details{margin-top:16px}
.solution-row{display:flex;justify-content:space-between;padding:9px 0;border-bottom:1px solid #e2e8f0;font-size:14px}
.solution-row:last-child{border-bottom:none}
.solution-row .label{color:#64748b}
.solution-row .value{font-family:monospace;font-weight:600}
.iterations-block{background:#fff;border:1px solid #e2e8f0;border-top:none;border-radius:0 0 8px 8px;padding:16px;margin-top:0}
.iteration{border:1px solid #e2e8f0;border-radius:8px;margin-bottom:10px;overflow:hidden}
.iteration-header{display:flex;justify-content:space-between;padding:10px 14px;background:#f1f5f9;cursor:pointer;font-size:13px;font-weight:500;user-select:none}
.iteration-header:hover{background:#e9edf2}
.iteration-header .meta{font-family:monospace;font-size:12px;color:#94a3b8}
.chevron{transition:transform .2s;font-size:10px;color:#94a3b8}
.iteration.open .chevron{transform:rotate(90deg)}
.iteration-body{display:none;padding:14px;border-top:1px solid #e2e8f0;overflow-x:auto}
.iteration.open .iteration-body{display:block}
.tableau-wrapper{overflow-x:auto}
.tableau{border-collapse:collapse;font-family:monospace;font-size:12px;white-space:nowrap}
.tableau th,.tableau td{padding:6px 10px;text-align:center;border:1px solid #e2e8f0}
.tableau th{background:#f0f2f5;font-weight:600;font-size:11px;color:#64748b}
.tableau td.pivot{background:#fef3c7;font-weight:700;color:#92400e}
.tableau td.basis-label{background:#eff6ff;color:#2563eb;font-weight:600;text-align:left}
.tableau td.ratio-cell{color:#94a3b8;font-style:italic}
.tableau td.iter-cell{background:#f1f5f9;font-weight:700;font-size:14px;vertical-align:middle}
.tableau tr.z-row td{border-top:2px solid #1e293b;background:#f8f9fb;font-weight:600}
.phase-separator{text-align:center;padding:8px 0;font-size:12px;font-weight:700;color:#2563eb;text-transform:uppercase;letter-spacing:.6px}
.error-banner{background:#fef2f2;border:1px solid #fecaca;border-radius:6px;padding:12px 16px;color:#dc2626;font-size:13px}
</style></head>
<body>
<h1>LIMO &mdash; Linear Method Optimizer Report</h1>
<p class="report-meta">Generated: ${now}</p>
<h2>Problem</h2><pre>${problemText}</pre>
<h2>Solution</h2><div class="solution-block">${solutionHtml}</div>
<h2>Simplex Iterations</h2><div class="iterations-block">${iterHtml}</div>
</body></html>`;
  }

  function generateTxtReport() {
    const lines = [];
    const hr = (ch = '-', n = 60) => ch.repeat(n);

    lines.push('LIMO — Linear Method Optimizer Report');
    lines.push(hr('='));
    lines.push(`Generated: ${new Date().toLocaleString()}`);
    lines.push('');

    lines.push('PROBLEM');
    lines.push(hr());
    lines.push($('preview').textContent);
    lines.push('');

    lines.push('SOLUTION');
    lines.push(hr());
    if (lastResult && lastResult.solution) {
      const sol = lastResult.solution;
      lines.push(`Status: ${sol.status.toUpperCase()}`);
      if (sol.status === 'optimal') {
        lines.push(`Objective Value: ${sol.objectiveValue}`);
        (sol.variableValues || []).forEach((v, i) => {
          lines.push(`  ${vn(i)} = ${v}`);
        });
      }
    }
    lines.push('');

    if (lastResult && lastResult.iterations && lastResult.iterations.length) {
      lines.push('SIMPLEX ITERATIONS');
      lines.push(hr());

      const bf = lastResult.basisFinding.result;
      const colHeaders = buildColHeaders(bf);
      const phases = new Set(lastResult.iterations.map(it => it.phase));
      const multiPhase = phases.size > 1;
      let currentPhase = -1;

      lastResult.iterations.forEach(it => {
        if (multiPhase && it.phase !== currentPhase) {
          currentPhase = it.phase;
          lines.push(`\n  Phase ${currentPhase}`);
          lines.push(hr('-', 40));
        }
        lines.push('');
        lines.push(`Iteration ${it.iterationNumber}   z = ${it.objectiveValue}`);
        lines.push(renderTableauTxt(it, colHeaders));
      });
    }

    return lines.join('\n');
  }

  function renderTableauTxt(it, allColHeaders) {
    const { tableau, basisVariables, pivotRow, pivotColumn, ratios } = it;
    const tableCols = tableau.cols;
    const constraintRows = tableau.data.slice(0, -1);
    const objectiveRow = tableau.data[tableau.data.length - 1];

    let varHeaders;
    if (tableCols === allColHeaders.length + 1) {
      varHeaders = [...allColHeaders];
    } else {
      varHeaders = [];
      for (let i = 0; i < tableCols - 1; i++) {
        varHeaders.push(allColHeaders[i] || `c${sub(i + 1)}`);
      }
    }
    const hasRatios = ratios && ratios.some(r => r !== null);

    const headers = ['I', 'Basis', ...varHeaders, 'Vb'];
    if (hasRatios) headers.push('Ratio');

    const dataRows = [];
    constraintRows.forEach((row, ri) => {
      const bv = basisVariables[ri];
      const basisLabel = bv !== undefined ? (allColHeaders[bv] || `c${sub(bv + 1)}`) : '?';
      const r = [ri === 0 ? String(it.iterationNumber) : '', basisLabel, ...row];
      if (hasRatios) r.push(ratios[ri] !== null ? String(ratios[ri]) : '-');
      dataRows.push(r);
    });

    const zRow = ['', 'z', ...objectiveRow];
    if (hasRatios) zRow.push('');
    dataRows.push(zRow);

    return asciiTable(headers, dataRows);
  }

  function asciiTable(headers, rows) {
    const cols = headers.length;
    const widths = headers.map((h, i) =>
      Math.max(String(h).length, ...rows.map(r => String(r[i] ?? '').length))
    );
    const sep = '+' + widths.map(w => '-'.repeat(w + 2)).join('+') + '+';
    const fmtRow = row =>
      '|' + row.map((cell, i) => ` ${String(cell ?? '').padEnd(widths[i])} `).join('|') + '|';
    return [sep, fmtRow(headers), sep, ...rows.map(fmtRow), sep].join('\n');
  }

  // ── Init ──

  function init() {
    $('dec-vars').addEventListener('click', () => changeVars(-1));
    $('inc-vars').addEventListener('click', () => changeVars(1));
    $('add-constraint').addEventListener('click', addConstraint);
    $('solve-btn').addEventListener('click', solve);
    $('obj-sense').addEventListener('change', updatePreview);
    $('basis-method').addEventListener('change', updatePreview);

    $('import-btn').addEventListener('click', () => $('file-import').click());
    $('file-import').addEventListener('change', e => {
      if (e.target.files[0]) importFile(e.target.files[0]);
      e.target.value = '';
    });
    $('export-html').addEventListener('click', () => downloadReport('html'));
    $('export-txt').addEventListener('click', () => downloadReport('txt'));
    $('export-json').addEventListener('click', () => downloadReport('json'));

    document.addEventListener('keydown', e => {
      if (e.key === 'Enter' && e.target.classList.contains('coeff-input')) {
        e.preventDefault();
        solve();
      }
    });

    // Task list event delegation
    $('tasks-list').addEventListener('click', e => {
      const dismissBtn = e.target.closest('.task-dismiss');
      if (dismissBtn) { dismissTask(dismissBtn.dataset.id); return; }
      const rowMain = e.target.closest('.task-item-main');
      if (rowMain) {
        const taskEl = rowMain.closest('.task-item');
        const t = tasks.find(t => t.id === taskEl?.dataset.id);
        if (t && t.state === 'SUCCESS') {
          if (selectedTaskId === t.id) {
            selectedTaskId = null;
            lastResult = null;
            $('iterations-card').classList.add('hidden');
            renderTaskList();
          } else {
            viewTask(t.id);
          }
        }
      }
    });
    $('clear-done-btn').addEventListener('click', clearDoneTasks);

    renderObjective();
    renderConstraints();
    renderExamples();
    updatePreview();
    startQueuePolling();
    startWorkersPolling();
  }

  document.readyState === 'loading'
    ? document.addEventListener('DOMContentLoaded', init)
    : init();
})();
