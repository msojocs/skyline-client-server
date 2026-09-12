export function createObjectManage(clazzMap = new Map<string, any>()) {
  return {
    getClazz: (name: string) => clazzMap.get(name),
    setClazz: (name: string, clazz: any) => { clazzMap.set(name, clazz); },
    removeClazz: (name: string) => { clazzMap.delete(name); },
    clearClazz: () => clazzMap.clear(),
    getAllClazz: () => Array.from(clazzMap.values()),
  };
}

export function createInstanceManage(instanceMap = new Map<number, any>()) {
  let objectIds = new WeakMap<object, number>();
  const primitiveIds = new Map<any, number>();
  let nextInstanceId = 1;

  const identityMap = (value: any) => (
    (typeof value === 'object' && value !== null) || typeof value === 'function'
      ? objectIds : primitiveIds
  );

  const removeInstance = (id: number) => {
    const instance = instanceMap.get(id);
    if (!instanceMap.delete(id)) return;
    const ids = identityMap(instance);
    if (ids.get(instance) !== id) return;
    ids.delete(instance);
    for (const [otherId, value] of instanceMap) {
      if (value === instance) {
        ids.set(instance, otherId);
        break;
      }
    }
  };

  return {
    getInstance: (id: number) => instanceMap.get(id),
    getInstanceId: (instance: any) => identityMap(instance).get(instance) ?? null,
    get instanceCount() { return instanceMap.size; },
    setInstance(instance: any) {
      const id = nextInstanceId++;
      instanceMap.set(id, instance);
      const ids = identityMap(instance);
      if (!ids.has(instance)) ids.set(instance, id);
      return id;
    },
    removeInstance,
    removeInstanceOfType(type: string) {
      for (const [id, instance] of instanceMap) {
        if (instance?.constructor?.name !== type) continue;
        instanceMap.delete(id);
        const ids = identityMap(instance);
        if (ids.get(instance) === id) ids.delete(instance);
      }
    },
    clearInstance() {
      instanceMap.clear();
      primitiveIds.clear();
      // IDs remain monotonic, but no identity from a closed connection survives.
      objectIds = new WeakMap();
    },
  };
}

export type ObjectManage = ReturnType<typeof createObjectManage>;
export type InstanceManage = ReturnType<typeof createInstanceManage>;
