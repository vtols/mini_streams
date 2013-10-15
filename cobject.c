#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define alloc_t(T) ((T *) malloc(sizeof(T)))
#define alloc(V) V = (typeof(V)) malloc(sizeof(typeof(*V)))

typedef struct write_stream write_stream;
typedef struct write_stream_operations write_stream_operations;
typedef struct aggregator_stream_data aggregator_stream_data;

struct write_stream_operations
{
    void    (*open)();
    void    (*write)(write_stream *self, char *s, size_t n);
    void    (*write_str)(write_stream *self, char *s);
    void    (*write_int)(write_stream *self, int n);
    void    (*close)(write_stream *self);
};

struct write_stream
{
    void *data;
    struct  write_stream_operations *w_op;
};

struct aggregator_stream_data
{
    size_t n;
    write_stream **streams;
};

write_stream *write_stream_new(write_stream_operations *stream_type)
{
    write_stream *w = alloc_t(write_stream);
    w->data = NULL;
    w->w_op = stream_type;
    return w;
}

void default_write_stream_open(write_stream *self)
{
}

void default_write_stream_write(write_stream *self, char *s, size_t n)
{
}

void default_write_stream_write_str(write_stream *self, char *s)
{
    self->w_op->write(self, s, strlen(s));
}

void default_write_stream_write_int(write_stream *self, int n)
{
    char buf[20];
    sprintf(buf, "%d", n);
    self->w_op->write_str(self, buf);
}

void default_write_stream_close(write_stream *self)
{
}

void file_write_stream_open(write_stream *self, char *path)
{
    self->data = fopen(path, "w");
}

void file_obj_write_stream_open(write_stream *self, FILE *f)
{
    self->data = f;
}

void file_obj_write_stream_write(write_stream *self, char *s, size_t n)
{
    fwrite(s, 1, n, self->data);
}

void file_obj_write_stream_close(write_stream *self)
{
    fclose(self->data);
}

void aggregator_write_stream_open(write_stream *self, size_t n,
                                    write_stream **ws)
{
    aggregator_stream_data *data;
    
    if (self->data)
        self->w_op->close(self);
        
    alloc(data);

    data->n = n;
    data->streams = ws;
    self->data = data;  
}

void aggregator_write_stream_write(write_stream *self, char *s, 
                                    size_t n)
{
    size_t i, k;
    aggregator_stream_data *data = self->data;
    
    k = data->n;
    for (i = 0; i < k; i++)
        data->streams[i]->w_op->write(data->streams[i], s, n);
}

void aggregator_write_stream_close(write_stream *self)
{
    free(self->data);
}

void mem_write_stream_open(write_stream *self, char *mem)
{
    self->data = mem;
}

void mem_write_stream_write(write_stream *self, char *s, size_t n)
{
    memcpy(self->data, s, n);
    self->data += n;
    *(char *)(self->data) = '\0';
}

#define WRITE_STREAM_DEFAULTS \
        .open       = default_write_stream_open, \
        .write      = default_write_stream_write, \
        .write_str  = default_write_stream_write_str, \
        .write_int  = default_write_stream_write_int, \
        .close      = default_write_stream_close,

write_stream_operations dummy_stream =
{
    WRITE_STREAM_DEFAULTS
};

write_stream_operations file_obj_stream = 
{
    WRITE_STREAM_DEFAULTS
    .open       = file_obj_write_stream_open,
    .write      = file_obj_write_stream_write,
    .close      = file_obj_write_stream_close,
};

write_stream_operations file_stream = 
{
    WRITE_STREAM_DEFAULTS
    .open       = file_write_stream_open,
    .write      = file_obj_write_stream_write,
    .close      = file_obj_write_stream_close,
};

write_stream_operations aggregator_stream = 
{
    WRITE_STREAM_DEFAULTS
    .open       = aggregator_write_stream_open,
    .write      = aggregator_write_stream_write,
    .close      = aggregator_write_stream_close,
};

write_stream_operations mem_stream = 
{
    WRITE_STREAM_DEFAULTS
    .open       = mem_write_stream_open,
    .write      = mem_write_stream_write,
};

#undef alloc_t
#undef alloc

int main()
{
    int i;
    char a[10000];
    write_stream *w   = write_stream_new(&file_stream),
                 *std = write_stream_new(&file_obj_stream),
                 *mem = write_stream_new(&mem_stream),
                 *agg = write_stream_new(&aggregator_stream),
                 *x[3];
    x[0] = w;
    x[1] = std;
    x[2] = mem;
    w->w_op->open(w, "abc.txt");
    std->w_op->open(std, stdout);
    mem->w_op->open(mem, a);
    agg->w_op->open(agg, 3, x);
    for (i = 0; i < 10; i++)
        agg->w_op->write_str(agg, "Hey hop!\n");
    w->w_op->close(w);
    agg->w_op->close(agg);
    puts("Memory:");
    puts(a);
    return 0;
}
