/// <reference types="./index.d.ts"/>

import _warthog from "./bin/warthog";
import _roadhog from "./bin/roadhog";
import _mapf from "./bin/mapf";

type Module = EmscriptenModule & {
  FS: typeof FS;
};

type Options = {
  args?: string[];
  preRun?: (m: Partial<Module>) => void;
  postRun?: (m: Partial<Module>) => void;
};

async function call(
  bin: (
    module: Partial<EmscriptenModule>,
    ...args: string[]
  ) => Promise<Module>,
  { args = [], preRun, postRun }: Options = {}
) {
  let stdout = "";
  let stderr = "";
  const module: Partial<EmscriptenModule> = {
    preRun: [() => preRun?.(module)],
    postRun: [() => postRun?.(module)],
    print: (a) => void (stdout += `${a}\n`),
    printErr: (a) => void (stderr += `${a}\n`),
    arguments: args,
  };
  await bin(module);
  return { stderr, stdout };
}

export const warthog = (args: Options) => call(_warthog, args);
export const roadhog = (args: Options) => call(_roadhog, args);
export const mapf = (args: Options) => call(_mapf, args);
