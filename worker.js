importScripts("action.js");

var Module = {
    print: (function() {
        return (...args) => {
            var text = args.join(' ');
            console.log(text);
        };
    })(),
    printErr: (function() {
        return (...args) => {
            var text = args.join(' ');
            console.log(text);
        };
    })(),
};
importScripts("out.js");

const get_trace_info = Module.cwrap('get_trace_info', 'string', ['string']);
const get_callstack_stats = Module.cwrap('get_callstack_stats', 'string',
	 				 ['string', 'string', 'string', 'string',
	 				  'string', 'string', 'string', 'string',
	 				  'string']);
const get_flamegraph = Module.cwrap('get_flamegraph', 'string',
				    ['string', 'string', 'string', 'string',
	 			     'string', 'string', 'string', 'string',
	 			     'string']);
const get_event_types = Module.cwrap('get_event_types', 'string', ['string']);

onmessage = (e) => {
    // console.log("Worker: Message received from main script", e.data);
    switch (e.data.action) {
    case ACTION.GET_TRACE_INFO: {
	const jsonRes = get_trace_info(e.data.dir);
	if (! jsonRes) {
	    // console.log(`Failed to read ${e.data.dir} as a CTF2 trace`)
	    const msg = {action: ACTION.GET_TRACE_INFO, rc: -1,
			 err: `Failed to read ${e.data.dir} as a CTF2 trace`};
	    postMessage(msg);
	    break;
	}
	const res = JSON.parse(jsonRes);
	_free(jsonRes);

	const msg = {action: ACTION.GET_TRACE_INFO, rc: 0, dir: e.data.dir, res: res};
	// console.log("Worker: Posting message back to main script", msg);
	postMessage(msg);
	break;
    }
    case ACTION.GET_CALLSTACK_STATS: {
	const t0 = performance.now();
	const jsonRes = get_callstack_stats(e.data.dir,
					    e.data.entry_regex, "cmp",
					    e.data.exit_regex, "cmp",
					    e.data.entry_track, e.data.exit_track,
					    e.data.entry_label, e.data.exit_label);
        if (! jsonRes) {
            // console.log(`get_callstack_stats errored on ${dir}`);
	    const msg = {action: ACTION.GET_CALLSTACK_STATS, rc: -1,
			 err: `callstack stats failed on ${dir}`};
	    postMessage(msg);
            break;
        }
        const res = JSON.parse(jsonRes);
        _free(jsonRes);
	const t1 = performance.now();
        console.log(`Call to get_callstack_stats took ${t1 - t0} milliseconds.`);
	const msg = {action: ACTION.GET_CALLSTACK_STATS, rc: 0, res: res, dur: t1 - t0};
	// console.log("Worker: Posting message back to main script", msg);
	postMessage(msg);
	break;
    }
    case ACTION.GET_FLAMEGRAPH: {
	const t0 = performance.now();
	const jsonRes = get_flamegraph(e.data.dir,
				       e.data.entry_regex, "cmp",
				       e.data.exit_regex, "cmp",
				       e.data.entry_track, e.data.exit_track,
				       e.data.entry_label, e.data.exit_label);
        if (! jsonRes) {
	    // console.log(`get_flamegraph errored on ${e.data.dir}`);
	    const msg = {action: ACTION.GET_FLAMEGRAPH, rc: -1,
			 err: `get_flamegraph failed on ${e.data.dir}`};
	    postMessage(msg);
            break;
        }
        const res = JSON.parse(jsonRes);
        _free(jsonRes);
	const t1 = performance.now();
        console.log(`Call to get_flamegraph took ${t1 - t0} milliseconds.`);
	const msg = {action: ACTION.GET_FLAMEGRAPH, rc: 0, res: res, dur: t1 - t0};
	// console.log("Worker: Posting message back to main script", msg);
	postMessage(msg);
	break;
    }
    case ACTION.WRITE_FILES: {
	try {
            Module.FS.mkdir(e.data.dir);
        } catch (err) {} // already created (hopefully)
	for (const f of e.data.files) {
	    // console.log(`Writing file to FS: ${f.file}`);
	    Module.FS.writeFile(f.file, new Uint8Array(f.data));
	}
	const msg = {action: ACTION.WRITE_FILES, rc: 0, dir: e.data.dir};
	// console.log("Worker: Posting message back to main script", msg);
	postMessage(msg);
	break;
    }
    case ACTION.GET_EVENT_TYPES: {
	// console.log("Calling get_event_type with arg:", e.data.dir);
        const t0 = performance.now();
        const jsonRes = get_event_types(e.data.dir);
        if (! jsonRes) {
            // console.log("get_event_types errored");
	    const msg = {action: ACTION.GET_EVENT_TYPES, rc: -1,
			 err: `get_flamegraph failed on ${e.data.dir}`};
	    postMessage(msg);
            return;
        }
        const res = JSON.parse(jsonRes);
        _free(jsonRes);
        const t1 = performance.now();
        console.log(`Call to get_event_types took ${t1 - t0} milliseconds.`);
	const msg = {action: ACTION.GET_EVENT_TYPES, rc: 0, res: res, dur: t1 - t0};
	// console.log("Worker: Posting message back to main script", msg);
	postMessage(msg);
	break;
    }
    default:
	console.log("unknown worker action");
    }
};
