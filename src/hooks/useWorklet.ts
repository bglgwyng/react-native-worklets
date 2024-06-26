import { DependencyList, useMemo } from "react";
import type { IWorkletContext } from "src/types";

/**
 * Create a Worklet function that persists between re-renders.
 * The returned function can be called from both a Worklet context and the JS context, but will execute on a Worklet context.
 *
 * @worklet
 * @param context The context to run this Worklet in. Can be `default` to use the default background context, or a custom context.
 * @param callback The Worklet. Must be marked with the `'worklet'` directive.
 * @param dependencyList The React dependencies of this Worklet.
 * @returns A memoized Worklet
 * ```ts
 * const sayHello = useWorklet('default', (name: string) => {
 *   'worklet'
 *   console.log(`Hello ${name}, I am running on the Worklet Thread!`)
 * }, [])
 * sayHello()
 * ```
 */
export function useWorklet<T extends (...args: any[]) => any>(
  context: IWorkletContext | "default",
  callback: T,
  dependencyList: DependencyList
): (...args: Parameters<T>) => Promise<ReturnType<T>> {
  const worklet = useMemo(
    () => {
      if (context === "default") {
        return Worklets.defaultContext.createRunAsync(callback);
      } else {
        return context.createRunAsync(callback);
      }
    },
    // eslint-disable-next-line react-hooks/exhaustive-deps
    dependencyList
  );

  return worklet;
}
