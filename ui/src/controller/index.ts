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

import {forwardRemoteCalls, Remote} from '../base/remote';
import {Action} from '../common/actions';
import {createEmptyState, State} from '../common/state';

const state: State = createEmptyState();

class Controller {
  private _frontend?: FrontendProxy;

  get frontend(): FrontendProxy {
    if (!this._frontend) throw new Error('No FrontendProxy');
    return this._frontend;
  }

  init(frontendProxyPort: MessagePort): void {
    this._frontend = new FrontendProxy(frontendProxyPort);
  }

  doAction(_: Action): void {
    state.i++;
    this.frontend.updateState(state);
  }
}

/**
 * Proxy for talking to the main thread.
 * TODO(hjd): Reduce the boilerplate.
 */
class FrontendProxy extends Remote {
  updateState(state: State) {
    return this.send<void>('updateState', [state]);
  }
}

function main() {
  const controller = new Controller();
  forwardRemoteCalls(self as {} as MessagePort, controller);
}

main();
