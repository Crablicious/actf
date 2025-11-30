const exampleTrace = "example";
let isExampleLoaded = false;
var eventChart = null;
const dirInput = document.getElementById("dir-input");
dirInput.style.display = "none";
document.getElementById('callstack-params').style.opacity = 0.6;
var traceInfo = null;

function resetSelector(selector) {
    while (selector.childNodes.length > 1) { selector.removeChild(selector.lastChild); }
    selector.childNodes[0].selected = true;
}

function onNewTraceLoadedPre() {
    document.getElementById('trace-info-status').innerText = "";
    document.getElementById('trace-info-attributes').innerHTML = "";
    traceInfo = null;

    document.getElementById('event-chart').style.display = "none";
    document.getElementById('run-event-types-button').disabled = true;
    document.getElementById("run-event-types-err").innerText = "";
    document.getElementById('run-callstack-stats-button').disabled = true;
    document.getElementById("run-callstack-stats-err").innerText = "";
    document.getElementById('run-flamegraph-button').disabled = true;
    document.getElementById("run-flamegraph-err").innerText = "";

    document.getElementById('callstack-params').style.opacity = 0.6;
    const entrySelect = document.getElementById('entry-select');
    entrySelect.disabled = true;
    resetSelector(entrySelect);
    const exitSelect = document.getElementById('exit-select');
    exitSelect.disabled = true;
    resetSelector(exitSelect);
    const entryTrackSelect = document.getElementById('entry-track-select');
    entryTrackSelect.disabled = true;
    resetSelector(entryTrackSelect);
    const exitTrackSelect = document.getElementById('exit-track-select');
    exitTrackSelect.disabled = true;
    resetSelector(exitTrackSelect);
    const entryLabelSelect = document.getElementById('entry-label-select');
    entryLabelSelect.disabled = true;
    resetSelector(entryLabelSelect);
    const exitLabelSelect = document.getElementById('exit-label-select');
    exitLabelSelect.disabled = true;
    resetSelector(exitLabelSelect);

    if (eventChart) {
        eventChart.destroy();
        eventChart = null;
    }
    document.getElementById('callstack-stats-container').innerHTML = "";
    document.getElementById('flamegraph-container').innerHTML = "";
}

function onNewTraceLoadedPost() {
    document.getElementById('run-event-types-button').disabled = false;
    document.getElementById('callstack-params').style.opacity = 1.0;
}

function addOptions(selectId, values) {
    const selector = document.getElementById(selectId);
    for (const value of values) {
	const opt = document.createElement("option");
	opt.value = value;
	opt.innerHTML = value;
	selector.appendChild(opt);
    }
}

function updateCallstackButtons() {
    const enabled = document.getElementById('entry-select').value &&
	  document.getElementById('exit-select').value &&
	  document.getElementById('entry-track-select').value &&
	  document.getElementById('exit-track-select').value &&
	  document.getElementById('entry-label-select').value &&
	  document.getElementById('exit-label-select').value;
    document.getElementById('run-callstack-stats-button').disabled = !enabled;
    document.getElementById('run-flamegraph-button').disabled = !enabled;
}

function updateFieldOptions(eventName, trackSelectId, labelSelectId) {
    const event = traceInfo.events.find((ev) => ev.name === eventName);
    if (event === undefined) {
	return;
    }
    const trackSelect = document.getElementById(trackSelectId);
    resetSelector(trackSelect);
    const labelSelect = document.getElementById(labelSelectId);
    resetSelector(labelSelect);
    for (const field of event["value-fields"]) {
	const opt = document.createElement("option");
	opt.value = field;
	opt.innerHTML = field;
	trackSelect.appendChild(opt);
	labelSelect.appendChild(opt.cloneNode(true));
    }
    trackSelect.disabled = false;
    labelSelect.disabled = false;
}

function updateEntryFieldOptions(value) {
    updateFieldOptions(value, 'entry-track-select', 'entry-label-select');
    updateCallstackButtons();
}

function updateExitFieldOptions(value) {
    updateFieldOptions(value, 'exit-track-select', 'exit-label-select');
    updateCallstackButtons();
}

function traceInfoString(data, indent) {
    text = "";
    for (const [key, value] of Object.entries(data)) {
        if (value !== null) {
            if (typeof value === 'object') {
                text += ' '.repeat(indent)+`${key}:\n`
                text += traceInfoString(value, indent + 2);
            } else {
                text += ' '.repeat(indent)+`${key}: ${value}\n`
            }
        }
    }
    return text;
}

function traceInfoToDl(dl, data) {
    for (const [key, value] of Object.entries(data)) {
        if (value !== null) {
            const dt = document.createElement("dt");
            dt.innerHTML = `${key}`;
            dl.appendChild(dt);
            if (typeof value === 'object') {
                const dd = document.createElement("dd");
                const innerDl = document.createElement("dl");
                traceInfoToDl(innerDl, value);
                dd.appendChild(innerDl);
                dl.appendChild(dd);
            } else {
                const dd = document.createElement("dd");
                dd.innerHTML = `${value}`;
                dl.appendChild(dd);
            }
        }
    }
}

function updateTraceInfo(data, dir) {
    const ti = document.getElementById('trace-info-status');
    ti.innerText = `Trace loaded successfully from directory ${dir}`;

    const attrs = document.getElementById('trace-info-attributes');

    const dl = document.createElement("dl");
    traceInfoToDl(dl, data);

    attrs.appendChild(dl);
}

function updateEventOptions(events) {
    const entrySelect = document.getElementById('entry-select');
    const exitSelect = document.getElementById('exit-select');
    const names = []
    for (const event of events) {
	const opt = document.createElement("option");
	opt.value = event.name;
	opt.innerHTML = event.name;
	entrySelect.appendChild(opt);
	exitSelect.appendChild(opt.cloneNode(true));
    }
    entrySelect.disabled = false;
    exitSelect.disabled = false;

    if (isExampleLoaded) {
	entrySelect.childNodes.forEach(function(ch) {
	    ch.selected = (ch.value === "func_entry");
	});
	updateEntryFieldOptions("func_entry");
	exitSelect.childNodes.forEach(function(ch) {
	    ch.selected = (ch.value === "func_exit");
	});
	updateExitFieldOptions("func_exit");

	document.getElementById('entry-track-select').childNodes.forEach(function(ch) {
	    ch.selected = (ch.value === "/common-context/tid");
	});
	document.getElementById('entry-label-select').childNodes.forEach(function(ch) {
	    ch.selected = (ch.value === "/payload/addr");
	});
	document.getElementById('exit-track-select').childNodes.forEach(function(ch) {
	    ch.selected = (ch.value === "/common-context/tid");
	});
	document.getElementById('exit-label-select').childNodes.forEach(function(ch) {
	    ch.selected = (ch.value === "/payload/addr");
	});
	updateCallstackButtons();
    }
}

function runTraceInfo(dir) {
    console.log(`Fetching trace info for ${dir}`);
    worker.postMessage({action: ACTION.GET_TRACE_INFO, dir: dir});
    return 0;
}

function updateChart(data) {
    const ctx = document.getElementById('event-chart');
    if (eventChart) {
        eventChart.destroy();
    }
    eventChart = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: data.map(r => r.event_name),
            datasets: [{
                label: '# of events',
                data: data.map(r => r.count),
                borderWidth: 1,
		backgroundColor: '#b0c5e6'
            }]
        },
        options: {
            scales: {
                y: {
                    beginAtZero: true
                }
            }
        }
    });
    ctx.style.display = "";
}

function updateCallstackStatsTable(data) {
    const con = document.getElementById('callstack-stats-container');
    const tbl = document.createElement("table");
    tbl.style.overflow = 'auto';
    tbl.style.height = '300px';
    tbl.style.display = 'block';
    tbl.style.border = '0px';

    const tblHead = document.createElement("thead");
    tblHead.style.position = 'sticky';
    tblHead.style.top = '0px';
    const hdrRow = document.createElement("tr");
    for (const hdr of data.hdrs) {
        const cell = document.createElement("th");
        cell.scope = "col";
        const cellText = document.createTextNode(hdr);
        cell.appendChild(cellText);
        hdrRow.appendChild(cell);
    }
    tblHead.appendChild(hdrRow);
    tbl.appendChild(tblHead);

    const tblBody = document.createElement("tbody");
    tblBody.style.whiteSpace = 'nowrap';

    const totalIdx = data.hdrs.indexOf("total");
    if (totalIdx >= 0) {
	data.rows.sort((a, b) => {
	    return b[4] - a[4];
	});
    }
    for (const datarow of data.rows) {
        const row = document.createElement("tr");

        for (var i = 0; i < datarow.length; i++) {
            let cell;
            if (i == 0) {
                cell = document.createElement("th");
                cell.scope = "row";
            } else {
                cell = document.createElement("td");
            }
            let cellText;
            if (datarow[i] === null) {
                cellText = document.createTextNode("");
            } else if (typeof datarow[i] == "number") {
                cellText = document.createTextNode(Math.round((datarow[i] + Number.EPSILON) * 100) / 100);
            } else {
                cellText = document.createTextNode(datarow[i]);
            }
            cell.appendChild(cellText);
            row.appendChild(cell);
        }

        tblBody.appendChild(row);
    }

    tbl.appendChild(tblBody);
    con.appendChild(tbl);
}

function updateFlamegraph(data) {
    // console.log("flamegraph data:", data);
    var chart = flamegraph()
	.width(960);
    d3.select("#flamegraph-container")
        .datum(data)
        .call(chart);
}

function pathToDir(path) {
    return path.split("/")[0];
}

function getInputDir() {
    if (isExampleLoaded) {
        return exampleTrace;
    }
    const dirInput = document.getElementById("dir-input");
    const files = document.getElementById("dir-input").files;
    if (! dirInput.value) {
        return null;
    } else if (dirInput.files.length == 0) {
        return null;
    }
    return pathToDir(dirInput.files[0].webkitRelativePath);
}

const reader = (file) =>
      new Promise((resolve, reject) => {
          // console.log("Reading file:", file.webkitRelativePath);
          const fr = new FileReader();
          fr.onload = () => {
	      resolve({file: file.webkitRelativePath, data: fr.result});
          }
          fr.onerror = (err) => reject(err);
          fr.readAsArrayBuffer(file);
      });

async function dumpFilesToFS(dir, fileList) {
    let fileResults = [];
    const frPromises = fileList
          .filter((file) => (file.webkitRelativePath.split('/').length - 1) == 1)
          .map(reader);
    fileResults = await Promise.all(frPromises);
    const files = [];
    for (const fr of fileResults) {
	files.push({file: fr.file, data: fr.data});
    }
    const msg = {action: ACTION.WRITE_FILES, dir: dir, files: files};
    worker.postMessage(msg);
}

dirInput.addEventListener(
    "change",
    async (event) => {
        isExampleLoaded = false;
        onNewTraceLoadedPre();
        if (! event.target.files) {
            return;
        }
        const dir = pathToDir(event.target.files[0].webkitRelativePath);
	// read all files into an object with:
	// <filename0>: ArrayBuffer
	// <filename1>: ArrayBuffer
	// dir: <dir>
	// The ArrayBuffers should be zero-copy transferred.
        console.log("User-provided dir:", dir);
        try {
            Module.FS.mkdir(dir);
        } catch (err) {} // already created (hopefully)
	try {
	    await dumpFilesToFS(dir, [...event.target.files]);
	} catch (e) {
	    console.error("Failed to read files:", e);
	}
    },
    false,
);
document.getElementById("example-input-button").addEventListener(
    "click",
    async (event) => {
        isExampleLoaded = true;
        onNewTraceLoadedPre();
        runTraceInfo(exampleTrace);
    }
);
document.getElementById("run-event-types-button").addEventListener(
    "click",
    async (event) => {
        document.getElementById("run-event-types-err").innerText = "";

        const dir = getInputDir();
        if (dir === null) {
            document.getElementById("run-event-types-err").innerText = "No valid input";
            return;
        }
	document.getElementById("event-types-loading-img").style.visibility = "visible";
	worker.postMessage({action: ACTION.GET_EVENT_TYPES, dir: dir});
    }
);
document.getElementById("run-callstack-stats-button").addEventListener(
    "click",
    async (event) => {
        document.getElementById("run-callstack-stats-err").innerText = "";
        document.getElementById("callstack-stats-container").innerHTML = "";

        const dir = getInputDir();
        if (dir === null) {
            document.getElementById("run-callstack-stats-err").innerText = "No valid input";
            return;
        }

        console.log("Calling get_callstack_stats with arg:", dir);
        const entry_regex = document.getElementById("entry-select").value;
        const entry_track = document.getElementById("entry-track-select").value;
        const entry_label = document.getElementById("entry-label-select").value;
        const exit_regex = document.getElementById("exit-select").value;
        const exit_track = document.getElementById("exit-track-select").value;
        const exit_label = document.getElementById("exit-label-select").value;
	document.getElementById("callstack-stats-loading-img").style.visibility = "visible";
	worker.postMessage({action: ACTION.GET_CALLSTACK_STATS, dir: dir,
			    entry_regex: entry_regex, exit_regex: exit_regex,
			    entry_track: entry_track, exit_track: exit_track,
			    entry_label: entry_label, exit_label: exit_label});
    }
);
document.getElementById("run-flamegraph-button").addEventListener(
    "click",
    async (event) => {
        document.getElementById("run-flamegraph-err").innerText = "";
        document.getElementById("flamegraph-container").innerHTML = "";

        const dir = getInputDir();
        if (dir === null) {
            document.getElementById("run-flamegraph-err").innerText = "No valid input";
            return;
        }
        console.log("Calling get_flamegraph with arg:", dir);
        const entry_regex = document.getElementById("entry-select").value;
        const entry_track = document.getElementById("entry-track-select").value;
        const entry_label = document.getElementById("entry-label-select").value;
        const exit_regex = document.getElementById("exit-select").value;
        const exit_track = document.getElementById("exit-track-select").value;
        const exit_label = document.getElementById("exit-label-select").value;
	document.getElementById("flamegraph-loading-img").style.visibility = "visible";
	worker.postMessage({action: ACTION.GET_FLAMEGRAPH, dir: dir,
			    entry_regex: entry_regex, exit_regex: exit_regex,
			    entry_track: entry_track, exit_track: exit_track,
			    entry_label: entry_label, exit_label: exit_label});
    }
);

const worker = new Worker("worker.js");
worker.onmessage = (e) => {
    // console.log("Message received from worker", e.data);
    switch (e.data.action) {
    case ACTION.GET_TRACE_INFO:
	if (e.data.rc === 0) {
	    traceInfo = e.data.res;
	    onNewTraceLoadedPost();
	    updateTraceInfo(e.data.res['trace-info'], e.data.dir);
	    updateEventOptions(e.data.res['events']);
	} else {
	    document.getElementById('trace-info-status').innerText = e.data.err;
	}
	break;
    case ACTION.GET_CALLSTACK_STATS: {
	const timeout = 'dur' in e.data && e.data.dur < 1000 ? 1000 - e.data.dur : 0;
	setTimeout(() => {
	    document.getElementById("callstack-stats-loading-img").style.visibility = "hidden";
	}, timeout);
	if (e.data.rc === 0) {
	    updateCallstackStatsTable(e.data.res);
	} else {
	    console.log(e.data.err);
	    document.getElementById("run-callstack-stats-err").innerText = e.data.err;
	}
	break;
    }
    case ACTION.GET_FLAMEGRAPH: {
	const timeout = 'dur' in e.data && e.data.dur < 1000 ? 1000 - e.data.dur : 0;
	setTimeout(() => {
	    document.getElementById("flamegraph-loading-img").style.visibility = "hidden";
	}, timeout);
	if (e.data.rc === 0) {
	    updateFlamegraph(e.data.res);
	} else {
	    console.log(e.data.err);
	    document.getElementById("run-flamegraph-err").innerText = e.data.err;
	}
	break;
    }
    case ACTION.WRITE_FILES:
	if (e.data.rc === 0) {
	    runTraceInfo(e.data.dir);
	} else {
	    console.log(e.data.err);
	}
	break;
    case ACTION.GET_EVENT_TYPES: {
	const timeout = 'dur' in e.data && e.data.dur < 1000 ? 1000 - e.data.dur : 0;
	setTimeout(() => {
	    document.getElementById("event-types-loading-img").style.visibility = "hidden";
	}, timeout);
	if (e.data.rc === 0) {
	    updateChart(e.data.res);
	} else {
	    console.log(e.data.err);
	    document.getElementById("run-event-types-err").innerText = e.data.err;
	}
	break;
    }
    default:
    }
};
