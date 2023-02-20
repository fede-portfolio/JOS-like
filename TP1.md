TP1: Memoria virtual en JOS
===========================

boot_alloc_pos
--------------
<<<<<<< HEAD
### Un cálculo manual de la primera dirección de memoria que devolverá boot_alloc() tras el arranque. Se puede calcular a partir del binario compilado (obj/kern/kernel), usando los comandos readelf y/o nm y operaciones matemáticas.

La primera vez que llamemos a boot_alloc se, el valor de memoria sera end, entonces obtengo este valor

```
readelf -s obj/kern/kernel | grep end
110: f0117950     0 NOTYPE  GLOBAL DEFAULT    6 end
```

o bien
=======
>>>>>>> catedra/tp4

```
nm obj/kern/kernel | grep end
f0117950 B end
```

Se observa que la memoria se inicia en `0xf0117950` , a partir de esta valor inicial se calcula próxima dirección de memoria 

` nextfree = ROUNDUP(nextfree + n, PGSIZE)` 

reemplazando las variables quedaría

`nextfree = ROUNDUP(0xf0117950, 4096)` 

`ROUNDUP = (0xf0117950 + 4096 - 1) - (f0117950 + 4096 - 1) % 4096`

Para realizar los cálculos pasamos `0xf0117950` a decimal lo que nos da `4027677008`. Entonces

```
ROUNDUP = (4027677008 + 4096 - 1) - (4027677008 + 4096 - 1) % 4096
ROUNDUP = 4027678720
```
paso a hexadecimal el valor obtenido y así obtengo la dirección de memoria que se va retornar

`nextfree = 0xf0118000`


### Una sesión de GDB en la que, poniendo un breakpoint en la función boot_alloc(), se muestre el valor devuelto en esa primera llamada, usando el comando GDB finish.

```
Desktop/Sistemas_operativos/JOS_1/sisop_2022a_g34_codino_prieto$ make gdb
gdb -q -s obj/kern/kernel -ex 'target remote 127.0.0.1:26000' -n -x .gdbinit
Reading symbols from obj/kern/kernel...
Remote debugging using 127.0.0.1:26000
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x0000fff0 in ?? ()
(gdb) b boot_alloc
Breakpoint 1 at 0xf0100b29: file kern/pmap.c, line 89.
(gdb) continue
Continuing.
The target architecture is assumed to be i386
=> 0xf0100b29 <boot_alloc>:	push   %ebp

Breakpoint 1, boot_alloc (n=65684) at kern/pmap.c:89
89	{
(gdb) p (void *) &end
$1 = (void *) 0xf0117950
(gdb) p nextfree
$2 = 0x0
(gdb) fin
Run till exit from #0  boot_alloc (n=65684) at kern/pmap.c:89
=> 0xf01025dd <mem_init+26>:	mov    %eax,0xf0117948
mem_init () at kern/pmap.c:142
142		kern_pgdir = (pde_t *) boot_alloc(PGSIZE);
Value returned is $3 = (void *) 0xf0118000
(gdb) 
```
Como se observa en la sesion de GDB nuestros calculos fueron correctos y en la primera llamada se devuelve `0xf0118000`


map_region_large
----------------
### ¿cuánta memoria se ahorró de este modo? (en KiB)

Inicialmente ahorremos 4KiB ya que dejamos de usar `entry_pgtable`. Luego ahorraremos 4 KiB más por cada large page utilizada oportunisticamente ya que reemplazan a las pages tables


### ¿es una cantidad fija, o depende de la memoria física de la computadora?

Esto es valido para arquitecturas x86 , o sea, 32 bits ya que las page table tendran el mismo tamanio de 4KiB. Para arquitecturas de 64 bits esto podria no funcionar

...

