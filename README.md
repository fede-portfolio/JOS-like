TP4: Sistema de archivos e intérprete de comandos
=================================================

caché de bloques
----------------

#### Se recomienda leer la función diskaddr() en el archivo fs/bc.c. Responder:

**¿Qué es super->s_nblocks?**

La variable `super` hace referencia al superbloque del file system.
El superbloque contiene la información de todo el file system. En particular, `s_nblocks` contiene la cantidad de bloques que tiene el file system.


**¿Dónde y cómo se configura este bloque especial?**

Se configura en la función `void opendisk(const char *name)` del archivo `fsformat.c`

Extrayendo el código de la función se puede ver como se configura el superbloque

```
super = alloc(BLKSIZE);              //Alloca el superbloque
super->s_magic = FS_MAGIC;          //Setea el número mágico
super->s_nblocks = nblocks;        //Setea el total de bloques
super->s_root.f_type = FTYPE_DIR;  //Setea como directorio al nodos raiz
strcpy(super->s_root.f_name, "/"); //Setea el nombre de la raiz
```
