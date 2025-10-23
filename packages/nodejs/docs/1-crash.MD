# 崩溃分析

使用nodejs启动，创建对象会直接崩溃。

```cpp
__int64 __fastcall sub_7FFC1E826EE2(__int64 *a1)
{
  __int64 v2; // rdi
  __int64 v3; // rax
  __int64 v4; // rax
  _QWORD *v5; // rcx
  _QWORD *v7; // [rsp+28h] [rbp-40h] BYREF
  char v8[24]; // [rsp+30h] [rbp-38h] BYREF

  v2 = *(_QWORD *)(*a1 + 8);
  v8::HandleScope::HandleScope(v8, v2);
  v3 = *(_QWORD *)(*a1 + 40);
  // Value::QuickIsNullOrUndefined  根据AI判断，是指针压缩处理
  if ( (*(_DWORD *)(*a1 + 40) & 3) != 1 // if (!I::HasHeapObjectTag(obj)) return false;
    || *(_WORD *)(*(unsigned int *)(v3 - 1) + (v3 & 0xFFFFFFFF00000000ui64) + 7) != 67
    || (*(_DWORD *)(v3 + 23) & 0xFFFFFFFE) != 10 )
  {
    sub_7FFC1E826F8E(&v7, v2, a1);
    v4 = *a1;
    v5 = v7;
    if ( !v7 )
      v5 = (_QWORD *)(v4 + 16);
    *(_QWORD *)(v4 + 24) = *v5;
  }
  return v8::HandleScope::~HandleScope(v8);
}
```

其中 `(_WORD *)(*(unsigned int *)(v3 - 1) + (v3 & 0xFFFFFFFF00000000ui64) + 7)` **计算得到的地址实际上指向无效内存区域**。

第一部分判断：
```cpp
//  (*(_DWORD *)(*a1 + 40) & 3) != 1
V8_INLINE static bool HasHeapObjectTag(const internal::Address value) {
    return (value & kHeapObjectTagMask/*3*/) == static_cast<Address>(kHeapObjectTag/*1*/);
}
```

第二部分判断：
```cpp
// *(_WORD *)(*(unsigned int *)(v3 - 1) + (v3 & 0xFFFFFFFF00000000ui64) + 7) != 67
if (I::GetInstanceType(obj) != I::kOddballType/*67*/) return false;

// (_WORD *)(*(unsigned int *)(v3 - 1) + (v3 & 0xFFFFFFFF00000000ui64) + 7)
// *(unsigned int *)(v3 - 1) + (v3 & 0xFFFFFFFF00000000ui64)
ReadTaggedPointerField(obj, kHeapObjectMapOffset/*0*/)
// xxx + 7
// 开压缩就是8，不开是12
return ReadRawField<uint16_t>(map, kMapInstanceTypeOffset/*8*/); // = map + 7
```

第三部分判断：
```cpp
// (*(_DWORD *)(v3 + 23) & 0xFFFFFFFE) != 10 
I::GetOddballKind(obj) == kExternalOneByteRepresentationTag
```

# 编译NodeJs

编译开启压缩的NodeJs，结果还是会挂掉。。。。。。

编写一个开启压缩的hello.node，执行正常，汇编逻辑基本一致。。。。。。

感觉底层哪里还有不一样的。