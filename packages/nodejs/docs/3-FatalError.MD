## 现象

执行后报错：
```
[INFO:ffi.cc(910)] SkylineApplication:initRuntimeHolderPtr _runtimeHolderPtr=1534394032240
[INFO:ffi.cc(910)] WaLoad:setupFFIFunctions
[INFO:runtime.cc(62)] WaLoad Runtime::Runtime 000001654149BED0

#
# Fatal error in HandleScope::HandleScope
# Entering the V8 API without proper locking in place
#

```

## 处理

```c++
void HandleScope::Initialize(Isolate* isolate) {
    i::Isolate* internal_isolate = reinterpret_cast<i::Isolate*>(isolate);
    
    // 额外添加的
    if (internal_isolate->thread_manager()->IsLockedByThread(v8::internal::ThreadId::Invalid())) {
        internal_isolate->thread_manager()->Lock();
    }
    
    // We do not want to check the correct usage of the Locker class all over the
    // place, so we do it only here: Without a HandleScope, an embedder can do
    // almost nothing, so it is enough to check in this central place.
    // We make an exception if the serializer is enabled, which means that the
    // Isolate is exclusively used to create a snapshot.
    Utils::ApiCheck(
        !v8::Locker::IsActive() ||
            internal_isolate->thread_manager()->IsLockedByCurrentThread() ||
            internal_isolate->serializer_enabled(),
        "HandleScope::HandleScope",
        "Entering the V8 API without proper locking in place");
  
    i::HandleScopeData* current = internal_isolate->handle_scope_data();
    isolate_ = internal_isolate;
    prev_next_ = current->next;
    prev_limit_ = current->limit;
    current->level++;
}
```

以上为报错的逻辑位置。

`Utils::ApiCheck` 参数1为 `false` 的时候会报错，一下3个条件，至少需要一个true；

经过打印log，发现 `thread_manager` 中所属的线程id为无效值，也就是没有上锁就开始使用了；

```c++
!v8::Locker::IsActive() /* always !true -> false */ ||
            internal_isolate->thread_manager()->IsLockedByCurrentThread() ||
            internal_isolate->serializer_enabled() /* 快照的时候为true */
```

于是，暂时粗暴的上锁，测试可用：

```c++
if (internal_isolate->thread_manager()->IsLockedByThread(v8::internal::ThreadId::Invalid())) {
    internal_isolate->thread_manager()->Lock();
}
```
