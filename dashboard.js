document.addEventListener('DOMContentLoaded', () => {
    const supabase = window.supabaseClient;

    // Protect dashboard
    (async () => {
        const { data: { session } } = await supabase.auth.getSession();
        if (!session) {
            window.location.href = "login.html";
            return;
        }
        window.currentUser = session.user;
        displayUserInfo();
    })();

    function displayUserInfo() {
        const userEl = document.getElementById('loggedUser');
        if (userEl && window.currentUser) {
            const name = window.currentUser.user_metadata?.full_name || window.currentUser.email;
            userEl.innerHTML = `
                <div class="user-details">
                    <strong>${name}</strong>
                    <small>${window.currentUser.email}</small>
                </div>
            `;
        }
    }

    // Logout
    const logoutBtn = document.getElementById('logoutBtn');
    if (logoutBtn) {
        logoutBtn.addEventListener('click', async () => {
            await supabase.auth.signOut();
            window.location.href = "login.html";
        });
    }

    // === EXISTING DASHBOARD LOGIC (preserved exactly) ===
    const CHANNEL_ID = 3399453;
    const READ_KEY = '03Y0EYTKSY2G4CS3';
    const ESP_API_BASE = 'https://provider-fantastic-viable.ngrok-free.dev';

    const ITEMS = [ /* ... your existing ITEMS array ... */ ];

    let chartMode = 'weight';
    let allData = null;
    let myChart = null;
    let isDark = true;

 
   <script>
  const CHANNEL_ID = 3399453;
  const READ_KEY   = '03Y0EYTKSY2G4CS3';

  // ── CHANGE 1: ngrok HTTPS URL instead of local IP ──
  const ESP_API_BASE = 'https://provider-fantastic-viable.ngrok-free.dev';

  const ITEMS = [
    { name: 'Garri',      weightField: 1, stockField: 2, color: '#00d4aa', max: 5000 },
    { name: 'Beans',     weightField: 3, stockField: 4, color: '#ff6b6b', max: 1200 },
    { name: 'Rice',     weightField: 5, stockField: 6, color: '#ffd93d', max: 3000 },
    { name: 'Detergent', weightField: 7, stockField: 8, color: '#6bcbff', max: 250  },
  ];

  let chartMode = 'weight';
  let allData   = null;
  let myChart   = null;
  let isDark    = true;

  // ── THEME TOGGLE ──
  function toggleTheme() {
    isDark = !isDark;
    document.documentElement.setAttribute('data-theme', isDark ? 'dark' : 'light');
    document.getElementById('themeBtn').textContent = isDark ? '🌙' : '☀️';
    if (myChart) {
      const tc = isDark ? '#94a3b8' : '#64748b';
      const gc = isDark ? 'rgba(30,45,69,0.5)' : 'rgba(200,215,230,0.5)';
      myChart.options.plugins.legend.labels.color = tc;
      myChart.options.scales.x.ticks.color = tc;
      myChart.options.scales.y.ticks.color = tc;
      myChart.options.scales.x.grid.color  = gc;
      myChart.options.scales.y.grid.color  = gc;
      myChart.options.plugins.tooltip.backgroundColor = isDark ? '#111827' : '#ffffff';
      myChart.options.plugins.tooltip.titleColor = isDark ? '#e2e8f0' : '#1a202c';
      myChart.options.plugins.tooltip.borderColor = isDark ? '#1e2d45' : '#d1dbe8';
      myChart.update();
    }
  }

  // ── CHART INIT ──
  const ctx = document.getElementById('mainChart').getContext('2d');
  myChart = new Chart(ctx, {
    type: 'line',
    data: { labels: [], datasets: [] },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: { mode: 'index', intersect: false },
      plugins: {
        legend: {
          labels: { color: '#94a3b8', font: { family: 'Space Mono', size: 11 }, boxWidth: 12, boxHeight: 12 }
        },
        tooltip: {
          backgroundColor: '#111827',
          borderColor: '#1e2d45',
          borderWidth: 1,
          titleColor: '#e2e8f0',
          bodyColor: '#94a3b8',
          titleFont: { family: 'Syne', weight: '700' },
          bodyFont: { family: 'Space Mono', size: 11 },
        }
      },
      scales: {
        x: {
          ticks: { color: '#64748b', font: { family: 'Space Mono', size: 10 }, maxTicksLimit: 8 },
          grid: { color: 'rgba(30,45,69,0.5)' },
        },
        y: {
          ticks: { color: '#64748b', font: { family: 'Space Mono', size: 10 } },
          grid: { color: 'rgba(30,45,69,0.5)' },
        }
      }
    }
  });

  setRange(7, document.querySelector('.quick-btn.active'));

  // ── DATE RANGE DROPDOWNS ──
  function populateDateDropdowns() {
    const now = new Date();
    const fromDay = document.getElementById('fromDay');
    const fromMonth = document.getElementById('fromMonth');
    const fromYear = document.getElementById('fromYear');
    const toDay = document.getElementById('toDay');
    const toMonth = document.getElementById('toMonth');
    const toYear = document.getElementById('toYear');

    const years = [];
    for (let y = now.getFullYear(); y >= 2020; y--) years.push(y);

    const months = [
      {v:1, n:'Jan'}, {v:2, n:'Feb'}, {v:3, n:'Mar'}, {v:4, n:'Apr'},
      {v:5, n:'May'}, {v:6, n:'Jun'}, {v:7, n:'Jul'}, {v:8, n:'Aug'},
      {v:9, n:'Sep'}, {v:10, n:'Oct'}, {v:11, n:'Nov'}, {v:12, n:'Dec'}
    ];

    function fill(select, items, valueKey = 'value', textKey = 'text') {
      select.innerHTML = '';
      items.forEach(it => {
        const opt = document.createElement('option');
        opt.value = it[valueKey];
        opt.textContent = it[textKey];
        select.appendChild(opt);
      });
    }

    const days = [];
    for (let d = 1; d <= 31; d++) days.push({value: d, text: String(d).padStart(2, '0')});

    fill(fromDay, days, 'value', 'text');
    fill(toDay, days, 'value', 'text');
    fill(fromMonth, months.map(m => ({value: m.v, text: m.n})), 'value', 'text');
    fill(toMonth, months.map(m => ({value: m.v, text: m.n})), 'value', 'text');
    fill(fromYear, years.map(y => ({value: y, text: String(y)})), 'value', 'text');
    fill(toYear, years.map(y => ({value: y, text: String(y)})), 'value', 'text');

    setRange(7, document.querySelector('.quick-btn.active'));
  }

  function getFromISO() {
    const d = new Date(
      Number(document.getElementById('fromYear').value),
      Number(document.getElementById('fromMonth').value) - 1,
      Number(document.getElementById('fromDay').value),
      0,0,0
    );
    return d.toISOString();
  }
  function getToISO() {
    const d = new Date(
      Number(document.getElementById('toYear').value),
      Number(document.getElementById('toMonth').value) - 1,
      Number(document.getElementById('toDay').value),
      23,59,59
    );
    return d.toISOString();
  }

  function setRange(days, btn) {
    document.querySelectorAll('.quick-btn').forEach(b => b.classList.remove('active'));
    if (btn) btn.classList.add('active');

    const now = new Date();
    let from = new Date(now);

    if (days === 0) {
      from = new Date('2024-01-01T00:00:00');
    } else {
      from.setDate(now.getDate() - days);
    }

    document.getElementById('fromDay').value = from.getDate();
    document.getElementById('fromMonth').value = from.getMonth() + 1;
    document.getElementById('fromYear').value = from.getFullYear();
    document.getElementById('toDay').value = now.getDate();
    document.getElementById('toMonth').value = now.getMonth() + 1;
    document.getElementById('toYear').value = now.getFullYear();
  }

  populateDateDropdowns();

  // ── FETCH DATA ──
  async function fetchData() {
    const fromVal = getFromISO();
    const toVal   = getToISO();
    if (!fromVal || !toVal) { setStatus('Please select both From and To dates.', 'error'); return; }

    const fromISO = new Date(fromVal).toISOString();
    const toISO   = new Date(toVal).toISOString();

    setStatus('Fetching data from ThingSpeak...', 'loading');
    document.getElementById('fetchBtn').disabled = true;

    try {
      const url = `https://api.thingspeak.com/channels/${CHANNEL_ID}/feeds.json` +
                  `?api_key=${READ_KEY}&start=${encodeURIComponent(fromISO)}&end=${encodeURIComponent(toISO)}&results=500`;
      const res  = await fetch(url);
      const json = await res.json();

      if (!json.feeds || json.feeds.length === 0) {
        setStatus('No data found in the selected date range. Try a wider range.', 'error');
        document.getElementById('fetchBtn').disabled = false;
        return;
      }

      allData = json.feeds;
      renderChart(allData);
      updateStatCards(allData);
      renderTable(allData);
      checkLowStock(allData);

      const fromStr = new Date(fromISO).toLocaleDateString();
      const toStr   = new Date(toISO).toLocaleDateString();
      setStatus(`Loaded ${allData.length} data point(s) from ${fromStr} to ${toStr}`, 'success');
      document.getElementById('chartSubtitle').textContent =
        `${allData.length} entries · ${fromStr} → ${toStr}`;
      document.getElementById('tableSubtitle').textContent =
        `${allData.length} records · ${fromStr} → ${toStr}`;
      document.getElementById('exportBtn').disabled = false;

    } catch (err) {
      setStatus('Error fetching data. Check your internet connection.', 'error');
      console.error(err);
    }
    document.getElementById('fetchBtn').disabled = false;
  }

  // ── TRIGGER ESP WEIGH ──
  // ── CHANGE 2: added ngrok-skip-browser-warning header ──
  async function triggerWeigh(itemIndex) {
    const itemName = ITEMS[itemIndex].name;
    const btn = document.querySelectorAll('.stat-weigh-btn')[itemIndex];
    if (btn) { btn.disabled = true; btn.textContent = '⏳ Weighing...'; }
    setStatus(`Triggering weigh for ${itemName}...`, 'loading');

    try {
      const response = await fetch(`${ESP_API_BASE}/weigh?item=${itemIndex}`, {
        method: 'GET',
        headers: {
          'ngrok-skip-browser-warning': 'true'  // ← bypasses ngrok warning page
        }
      });

      if (!response.ok) {
        throw new Error(`HTTP ${response.status} ${response.statusText}`);
      }

      const data = await response.json();
      setStatus(
        `${itemName} weighed successfully — ${data.weight_g}g. Refreshing data...`,
        'success'
      );

      // Refresh ThingSpeak data after a short delay
      setTimeout(() => fetchData(), 2500);

    } catch (err) {
      setStatus(
        `Could not reach ESP for ${itemName}. Is ngrok still running? (${err.message})`,
        'error'
      );
      console.error('triggerWeigh error:', err);
    } finally {
      if (btn) { btn.disabled = false; btn.textContent = '⚖ Weigh Item'; }
    }
  }

  // ── CHART RENDER ──
  function renderChart(feeds) {
    const labels = feeds.map(f => {
      const d = new Date(f.created_at);
      return d.toLocaleDateString('en-GB', { day:'2-digit', month:'short' }) +
             ' ' + d.toLocaleTimeString('en-GB', { hour:'2-digit', minute:'2-digit' });
    });

    const datasets = ITEMS.map(item => {
      const fieldKey = chartMode === 'weight' ? `field${item.weightField}` : `field${item.stockField}`;
      return {
        label: item.name,
        data: feeds.map(f => f[fieldKey] ? parseFloat(f[fieldKey]) : null),
        borderColor: item.color,
        backgroundColor: item.color + '18',
        pointBackgroundColor: item.color,
        pointRadius: feeds.length < 20 ? 5 : 3,
        pointHoverRadius: 7,
        borderWidth: 2,
        tension: 0.3,
        spanGaps: true,
      };
    });

    myChart.data.labels   = labels;
    myChart.data.datasets = datasets;
    myChart.options.scales.y.title = {
      display: true,
      text: chartMode === 'weight' ? 'Weight (g)' : 'Stock (%)',
      color: '#64748b',
      font: { family: 'Space Mono', size: 10 }
    };
    myChart.update();
  }

  // ── STAT CARDS ──
  function updateStatCards(feeds) {
    const ids = [
      { wId:'garriWt',  pId:'garriPct',  bId:'garriBar',  sId:'garriScan',  wf:'field1', sf:'field2' },
      { wId:'beansWt', pId:'beansPct', bId:'beansBar', sId:'beansScan', wf:'field3', sf:'field4' },
      { wId:'riceWt', pId:'ricePct', bId:'riceBar', sId:'riceScan', wf:'field5', sf:'field6' },
      { wId:'detWt',   pId:'detPct',   bId:'detBar',   sId:'detScan',   wf:'field7', sf:'field8' },
    ];

    ids.forEach(item => {
      let weight = null, stock = null, lastTime = null;
      for (let i = feeds.length - 1; i >= 0; i--) {
        if (weight === null && feeds[i][item.wf]) {
          weight   = parseFloat(feeds[i][item.wf]);
          lastTime = new Date(feeds[i].created_at);
        }
        if (stock === null && feeds[i][item.sf]) stock = parseFloat(feeds[i][item.sf]);
        if (weight !== null && stock !== null) break;
      }

      if (weight !== null) {
        document.getElementById(item.wId).innerHTML = `${weight.toFixed(1)} <span>g</span>`;
      }
      if (stock !== null) {
        document.getElementById(item.pId).textContent = `Stock: ${stock.toFixed(0)}%`;
        document.getElementById(item.bId).style.width = `${Math.min(stock, 100)}%`;
      }
      if (lastTime) {
        const timeStr = lastTime.toLocaleDateString('en-GB') + ' ' +
                        lastTime.toLocaleTimeString('en-GB', { hour:'2-digit', minute:'2-digit' });
        document.getElementById(item.sId).innerHTML = `Last scan: <strong>${timeStr}</strong>`;
      }
    });
  }

  // ── LOW STOCK ALERT ──
  function checkLowStock(feeds) {
    const stockFields = ['field2','field4','field6','field8'];
    const names = ['Garri','Beans','Rice','Detergent'];
    const lowItems = [];

    names.forEach((name, i) => {
      let stock = null;
      for (let j = feeds.length - 1; j >= 0; j--) {
        if (feeds[j][stockFields[i]]) { stock = parseFloat(feeds[j][stockFields[i]]); break; }
      }
      if (stock !== null && stock < 20) lowItems.push(`${name} (${stock.toFixed(0)}%)`);
    });

    const banner = document.getElementById('alertBanner');
    const itemEl = document.getElementById('alertItems');
    if (lowItems.length > 0) {
      itemEl.innerHTML = lowItems.map(n => `<span class="alert-tag">${n}</span>`).join('');
      banner.classList.add('show');
    } else {
      banner.classList.remove('show');
    }
  }

  // ── TABLE ──
  function renderTable(feeds) {
    const tbody = document.getElementById('tableBody');
    tbody.innerHTML = '';
    feeds.forEach((f, i) => {
      const tr = document.createElement('tr');
      const d = new Date(f.created_at);

      function stockClass(val) {
        if (!val) return '';
        const n = parseFloat(val);
        if (n < 20) return 'td-low';
        if (n < 50) return 'td-mod';
        return 'td-ok';
      }

      tr.innerHTML = `
        <td>${i+1}</td>
        <td>${d.toLocaleString()}</td>
        <td>${f.field1 ? parseFloat(f.field1).toFixed(1) : '--'}</td>
        <td class="${stockClass(f.field2)}">${f.field2 ? parseFloat(f.field2).toFixed(0)+'%' : '--'}</td>
        <td>${f.field3 ? parseFloat(f.field3).toFixed(1) : '--'}</td>
        <td class="${stockClass(f.field4)}">${f.field4 ? parseFloat(f.field4).toFixed(0)+'%' : '--'}</td>
        <td>${f.field5 ? parseFloat(f.field5).toFixed(1) : '--'}</td>
        <td class="${stockClass(f.field6)}">${f.field6 ? parseFloat(f.field6).toFixed(0)+'%' : '--'}</td>
        <td>${f.field7 ? parseFloat(f.field7).toFixed(1) : '--'}</td>
        <td class="${stockClass(f.field8)}">${f.field8 ? parseFloat(f.field8).toFixed(0)+'%' : '--'}</td>
      `;
      tbody.appendChild(tr);
    });
  }

  // ── EXPORT (placeholder) ──
  function exportExcel() { alert('Export not implemented in this version.'); }

  // ── STATUS ──
  function setStatus(msg, type) {
    const s = document.getElementById('statusBar');
    s.className = 'status-bar';
    if (type === 'loading') s.classList.add('loading');
    if (type === 'error') s.classList.add('error');
    if (type === 'success') s.classList.add('success');
    s.textContent = msg;
  }

  // ── CHART SWITCH ──
  function switchChart(mode, btn) {
    chartMode = mode;
    document.querySelectorAll('.toggle-btn').forEach(b => b.classList.remove('active'));
    if (btn) btn.classList.add('active');
    if (allData) renderChart(allData);
  }
</script>
  <script>

const supabase = window.supabase.createClient(
    window.SUPABASE_URL,
    window.SUPABASE_ANON_KEY
);
let currentUser = null;
async function protectDashboard(){
    const { data, error } =
        await supabase.auth.getSession();
    if(error){
        console.error(error);
        location.href="login.html";
        return;
    }
    if(!data.session){
        location.href="login.html";
        return;
    }
    currentUser = data.session.user;
    document.getElementById("userMenu").style.display="flex";
    document.getElementById("userEmail").textContent =
        currentUser.email;
    const fullName =
        currentUser.user_metadata?.full_name;
    document.getElementById("userName").textContent =
        fullName || "User";
}
protectDashboard();
</script>
<script>
document
.getElementById("logoutBtn")
.addEventListener("click", async ()=>{
    await supabase.auth.signOut();
    location.href="login.html";
});
</script>
  <script>
supabase.auth.onAuthStateChange((event)=>{
    if(event==="SIGNED_OUT"){
        location.href="login.html";
    }
});
</script>
)
});
