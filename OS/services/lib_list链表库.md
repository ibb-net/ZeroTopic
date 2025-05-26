
# 简介


lib_list 是一个基于 FreeRTOS 列表功能构建的双链表管理库，提供简单易用的链表操作接口。该库使用两个链表（使用链表和空闲链表）来管理数据，实现高效的内存管理和数据操作。

# 数据结构




```c
typedef struct {
    size_t item_num;       // 链表最大项数
    size_t pre_item_size;  // 每项大小(字节)
    size_t counter;        // 当前使用项数
    List_t free_list;      // 空闲链表
    List_t use_list;       // 已使用链表
    ListItem_t *item_list; // 项目列表数组
    void *buffer;          // 数据缓冲区
} list_buffer_struct;

typedef list_buffer_struct *list_buffer_t;

```



# 主要功能

### 创建和销毁

 


```c
// 创建链表，指定最大项数和每项大小
list_buffer_t xCreateList(size_t item_num, size_t pre_item_size);

// 销毁链表，释放资源
void vDeleteList(list_buffer_t list_handle);

```

### 数据操作


```c
// 插入数据到链表
void *vInsertList(list_buffer_t list_handle, void *data);

// 获取指定索引位置的数据
void *vGetItem(list_buffer_t list_handle, size_t index);

// 获取链表中最后一项数据
void *vGetLastItem(list_buffer_t list_handle);

// 删除链表中最后一项数据
size_t vDeleteLastItem(list_buffer_t list_handle);

```

### 状态查询


```c
// 获取链表当前项数
size_t vGetListNum(list_buffer_t list_handle);

// 获取每项大小
size_t vGetItemPerSize(list_buffer_t list_handle);

// 获取链表最大容量
size_t vGetItemMaxNum(list_buffer_t list_handle);

```

### 使用示例


```c
// 创建一个可存储10个整数的链表
list_buffer_t my_list = xCreateList(10, sizeof(int));

// 插入数据
int value = 42;
vInsertList(my_list, &value);

// 获取最后一项数据
int *last_item = (int *)vGetLastItem(my_list);

// 获取链表当前项数
size_t count = vGetListNum(my_list);

// 删除最后一项
vDeleteLastItem(my_list);

// 销毁链表
vDeleteList(my_list);

```

## 注意事项

- 所有函数都会对传入的列表句柄进行非空检查

- 当链表已满时，vInsertList 会返回NULL

- 当链表为空时，vGetLastItem 和 vGetItem 会返回NULL

- 索引超出范围时，vGetItem 会返回NULL


