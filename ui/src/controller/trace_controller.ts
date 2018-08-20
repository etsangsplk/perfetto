// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

import '../tracks/all_controller';

import {assertExists, assertTrue} from '../base/logging';
import {
  Action,
  addChromeSliceTrack,
  addTrack,
  navigate,
  setEngineReady,
  setTraceTime,
  updateStatus,
  setVisibleTraceTime
} from '../common/actions';
import {TimeSpan} from '../common/time';
import {QuantizedLoad, ThreadDesc} from '../frontend/globals';
import {SLICE_TRACK_KIND} from '../tracks/chrome_slices/common';
import {CPU_SLICE_TRACK_KIND} from '../tracks/cpu_slices/common';

import {Child, Children, Controller} from './controller';
import {Engine} from './engine';
import {globals} from './globals';
import {QueryController, QueryControllerArgs} from './query_controller';
import {TrackControllerArgs, trackControllerRegistry} from './track_controller';

type States = 'init'|'loading_trace'|'ready';
export class TraceController extends Controller<States> {
  private readonly engineId: string;
  private engine?: Engine;

  constructor(engineId: string) {
    super('init');
    this.engineId = engineId;
  }

  run() {
    const engineCfg = assertExists(globals.state.engines[this.engineId]);
    switch (this.state) {
      case 'init':
        globals.dispatch(setEngineReady(this.engineId, false));
        this.loadTrace().then(() => {
          globals.dispatch(setEngineReady(this.engineId, true));
        });
        globals.dispatch(updateStatus('Opening trace'));
        this.setState('loading_trace');
        break;

      case 'loading_trace':
        if (this.engine === undefined || !engineCfg.ready) return;
        this.setState('ready');
        break;

      case 'ready':
        // At this point we are ready to serve queries and handle tracks.
        const engine = assertExists(this.engine);
        assertTrue(engineCfg.ready);
        const childControllers: Children = [];

        // Create a TrackController for each track.
        for (const trackId of Object.keys(globals.state.tracks)) {
          const trackCfg = globals.state.tracks[trackId];
          if (!trackControllerRegistry.has(trackCfg.kind)) continue;
          const trackCtlFactory = trackControllerRegistry.get(trackCfg.kind);
          const trackArgs: TrackControllerArgs = {trackId, engine};
          childControllers.push(Child(trackId, trackCtlFactory, trackArgs));
        }

        // Create a QueryController for each query.
        for (const queryId of Object.keys(globals.state.queries)) {
          const queryArgs: QueryControllerArgs = {queryId, engine};
          childControllers.push(Child(queryId, QueryController, queryArgs));
        }

        return childControllers;

      default:
        throw new Error(`unknown state ${this.state}`);
    }
    return;
  }

  private async loadTrace() {
    const engineCfg = assertExists(globals.state.engines[this.engineId]);
    let blob: Blob;
    if (engineCfg.source instanceof File) {
      blob = engineCfg.source;
    } else {
      globals.dispatch(updateStatus('Fetching trace from network'));
      blob = await(await fetch(engineCfg.source)).blob();
    }

    globals.dispatch(updateStatus('Parsing trace'));
    this.engine = await globals.createEngine(blob);
    const traceTime = await this.engine.getTraceTimeBounds();
    const actions = [
      setTraceTime(traceTime),
      navigate('/viewer'),
    ];

    if (globals.state.visibleTraceTime.lastUpdate === 0) {
      actions.push(setVisibleTraceTime(traceTime));
    }

    globals.dispatchMultiple(actions);

    await this.listTracks();
    await this.listThreads();
    await this.loadTimelineOverview(traceTime);
  }

  private async listTracks() {
    globals.dispatch(updateStatus('Loading tracks'));
    const engine = assertExists<Engine>(this.engine);
    const addToTrackActions: Action[] = [];
    const numCpus = await engine.getNumberOfCpus();
    for (let cpu = 0; cpu < numCpus; cpu++) {
      addToTrackActions.push(
          addTrack(this.engineId, CPU_SLICE_TRACK_KIND, cpu));
    }

    const threadQuery = await engine.rawQuery({
      sqlQuery: 'select upid, utid, tid, thread.name, max(slices.depth) ' +
          'from thread inner join slices using(utid) group by utid'
    });
    for (let i = 0; i < threadQuery.numRecords; i++) {
      const upid = threadQuery.columns[0].longValues![i];
      const utid = threadQuery.columns[1].longValues![i];
      const threadId = threadQuery.columns[2].longValues![i];
      let threadName = threadQuery.columns[3].stringValues![i];
      threadName += `[${threadId}]`;
      const maxDepth = threadQuery.columns[4].longValues![i];
      addToTrackActions.push(addChromeSliceTrack(
          this.engineId,
          SLICE_TRACK_KIND,
          upid as number,
          utid as number,
          threadName,
          maxDepth as number));
    }
    globals.dispatchMultiple(addToTrackActions);
  }

  private async listThreads() {
    globals.dispatch(updateStatus('Reading thread list'));
    const sqlQuery = 'select utid, tid, pid, thread.name, process.name ' +
        'from thread inner join process using(upid)';
    const threadRows = await assertExists(this.engine).rawQuery({sqlQuery});
    const threads: ThreadDesc[] = [];
    for (let i = 0; i < threadRows.numRecords; i++) {
      const utid = threadRows.columns[0].longValues![i] as number;
      const tid = threadRows.columns[1].longValues![i] as number;
      const pid = threadRows.columns[2].longValues![i] as number;
      const threadName = threadRows.columns[3].stringValues![i];
      const procName = threadRows.columns[4].stringValues![i];
      threads.push({utid, tid, threadName, pid, procName});
    }  // for (record ...)
    globals.publish('Threads', threads);
  }

  private async loadTimelineOverview(traceTime: TimeSpan) {
    const engine = assertExists<Engine>(this.engine);
    const numSteps = 100;
    const stepSec = traceTime.duration / numSteps;
    for (let step = 0; step < numSteps; step++) {
      globals.dispatch(updateStatus(
          'Loading overview ' +
          `${Math.round((step + 1) / numSteps * 1000) / 10}%`));
      const startSec = traceTime.start + step * stepSec;
      const startNs = Math.floor(startSec * 1e9);
      const endSec = startSec + stepSec;
      const endNs = Math.ceil(endSec * 1e9);

      // Sched overview.
      const schedRows = await engine.rawQuery({
        sqlQuery: `select sum(dur)/${stepSec}/1e9, cpu from sched ` +
            `where ts >= ${startNs} and ts < ${endNs} ` +
            'group by cpu order by cpu'
      });
      const schedData: {[key: string]: QuantizedLoad} = {};
      for (let i = 0; i < schedRows.numRecords; i++) {
        const load = schedRows.columns[0].doubleValues![i];
        const cpu = schedRows.columns[1].longValues![i] as number;
        schedData[cpu] = {startSec, endSec, load};
      }  // for (record ...)
      globals.publish('OverviewData', schedData);

      // Slices overview.
      const slicesRows = await engine.rawQuery({
        sqlQuery:
            `select sum(dur)/${stepSec}/1e9, process.name, process.pid, upid ` +
            'from slices inner join thread using(utid) ' +
            'inner join process using(upid) where depth = 0 ' +
            `and ts >= ${startNs} and ts < ${endNs} ` +
            'group by upid'
      });
      const slicesData: {[key: string]: QuantizedLoad} = {};
      for (let i = 0; i < slicesRows.numRecords; i++) {
        const load = slicesRows.columns[0].doubleValues![i];
        let procName = slicesRows.columns[1].stringValues![i];
        const pid = slicesRows.columns[2].longValues![i];
        procName += ` [${pid}]`;
        slicesData[procName] = {startSec, endSec, load};
      }
      globals.publish('OverviewData', slicesData);
    }  // for (step ...)
  }
}
