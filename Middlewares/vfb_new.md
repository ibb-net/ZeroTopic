# vfb_new 模块说明（规划）

## 目标
- 以回调订阅 + Topic 规则替换现有 VFB 队列多播方案。
- 提供任务/ISR 安全发布、对象字典联动、事件优先级/背压策略（可配置）。

## 对外接口（草案）
```c
int vfb_new_init(void);
int vfb_new_subscribe(obj_dict_key_t key,
                      void (*cb)(obj_dict_key_t key, const void* data, size_t len, uint64_t ts, void* user),
                      void* user);
int vfb_new_unsubscribe(obj_dict_key_t key, void (*cb)(obj_dict_key_t, const void*, size_t, uint64_t, void*), void* user);
int vfb_new_publish(obj_dict_key_t key, const void* data, size_t len);      // 任务上下文
int vfb_new_publish_isr(obj_dict_key_t key, const void* data, size_t len);  // ISR 上下文
```

## 迁移建议
- 保留旧 `vfb.h` 适配层，逐步将 `vfb_send/vfb_subscribe` 代理到新实现，支持平滑过渡。

## 与 microROS 的关系
- 可对接 microROS Publisher/Subscriber，作为 Node 内部总线或桥接层。


