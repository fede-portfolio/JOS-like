TP2: Procesos de usuario
========================

env_alloc
---------
### Se pide leer la función env_alloc() en su totalidad y responder las siguientes preguntas:

### 1. ¿Qué identificadores se asignan a los primeros 5 procesos creados? (Usar base hexadecimal)

El código para genera los env_id es el siguiente:

```
// Generate an env_id for this environment.
generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
if (generation <= 0)  // Don't create a negative env_id.
	generation = 1 << ENVGENSHIFT;
e->env_id = generation | (e - envs);

```
Para poder resolver la operación matemática definimos las variables utilizadas.

Cuando se inicializa un `env` su `env->env_id = 0`

```
ENVGSHIFT = 12
NENV 1 << 10 (1024)
```

La operación entre `e - envs` es una operación de aritmética de punteros, donde `e` apunta a la dirección de memoria del `env` que se esta inicializando y `envs` apunta a la dirección de memoria del arreglo que contiene todos los `env`.

Tomando estos valores las cuentas quedarían:

```
generation = (0 + (1 << 12)) & ~(1 <<10 - 1)
generation = 4096 = (0x1000)
```

Ahora reemplazo en la segunda parte de la operación para los 5 primeros procesos creados:
```
e->env_id = generation | (e - envs)

0. e->env_id = generation | 0 = 4096 | 0 = 4096 
1. e->env_id = generation | 1 = 4096 | 1 = 4097 
2. e->env_id = generation | 2 = 4096 | 2 = 4098
3. e->env_id = generation | 3 = 4096 | 3 = 4099 
4. e->env_id = generation | 4 = 4096 | 4 = 4100  
```

Paso los valores a hexadecimal y quedan: 

```
0. e->env_id = 0x1000
1. e->env_id = 0x1001 
2. e->env_id = 0x1002 
3. e->env_id = 0x1003 
4. e->env_id = 0x1004 
```


### 2. Supongamos que al arrancar el kernel se lanzan NENV procesos a ejecución. A continuación, se destruye el proceso asociado a envs[630] y se lanza un proceso que cada segundo, muere y se vuelve a lanzar (se destruye, y se vuelve a crear). ¿Qué identificadores tendrán esos procesos en las primeras cinco ejecuciones?

utilizamos la misma fórmula anterior y tenemos en cuenta que ahora lo que va a cambiar es el generation y el offset se va a mantener constante

```
((0 + (1 << 12) & ~((1 << 10) - 1)) | 630) = 4726 = 0x1276
((4726 + (1 << 12) & ~((1 << 10) - 1)) | 630) = 8822 = 0x2276
((8822 + (1 << 12) & ~((1 << 10) - 1)) | 630) = 12918 = 0x3276
((12918 + (1 << 12) & ~((1 << 10) - 1)) | 630) = 17014 = 0x4276
((17014 + (1 << 12) & ~((1 << 10) - 1)) | 630) = 21110 = 0x5276

```
Entonces

```
0. e->env_id = 0x1276
1. e->env_id = 0x2276
2. e->env_id = 0x3276
3. e->env_id = 0x4276
1. e->env_id = 0x5276
```


...


env_pop_tf
----------

### 1. Dada la secuencia de instrucciones assembly en la función, describir qué contiene durante su ejecución:

##### el tope de la pila justo antes popal
El tope de la pila contiene al trapframe recibido por parámetro

##### el tope de la pila justo antes iret
El tope de la pila contiene el `uintptr_t tf_eip` , es decir el instruction pointer del trapframe recibido

##### el tercer elemento de la pila justo antes de iret
El tercer elementos es `uint32_t tf_eflags`, es decir los flags del trapframe recibido

### 2. En la documentación de iret en [IA32-2A] se dice:

    If the return is to another privilege level, the IRET instruction also pops the stack pointer and SS from the stack, before resuming program execution.

### ¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)? Ayuda: Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?

El privelegio actual se guarda en CPL y son 2 bits.

La CPU determina si hubo un cambio de ring comparando el CPL del trapframe con el del code segment actual

...


gdb_hello
---------
### 1. Poner un breakpoint en env_pop_tf() y continuar la ejecución hasta allí.

```
(gdb) b env_pop_tf
Breakpoint 1 at 0xf0102ecb: file kern/env.c, line 482.
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0xf0102ecb <env_pop_tf>:	endbr32 

Breakpoint 1, env_pop_tf (tf=0xf01c7000) at kern/env.c:482
482	{
(gdb) 
```


### 2. En QEMU, entrar en modo monitor (Ctrl-a c), y mostrar las cinco primeras líneas del comando info registers.

```
(qemu) info registers
EAX=003bc000 EBX=f01c7000 ECX=f03bc000 EDX=00000214
ESI=00010094 EDI=00000000 EBP=f0118fd8 ESP=f0118fbc
EIP=f0102ecb EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
```

### 3. De vuelta a GDB, imprimir el valor del argumento tf:

```
(gdb)  p tf
$1 = (struct Trapframe *) 0xf01c7000
```



### 4. Imprimir, con x/Nx tf tantos enteros como haya en el struct Trapframe donde N = sizeof(Trapframe) / sizeof(int).

(Se puede calcular a mano afuera de GDB, o mediante el comando: print sizeof(struct Trapframe) / sizeof(int), utilizando ese resultado en x/Nx tf)

```
(gdb) print sizeof(struct Trapframe) / sizeof(int)
$2 = 17

(gdb) x/17x tf
0xf01c7000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c7030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c7040:	0x00000023
```

### 5. Avanzar hasta justo después del movl ...,%esp, usando si M para ejecutar tantas instrucciones como sea necesario en un solo paso:

```
(gdb) disas
Dump of assembler code for function env_pop_tf:
=> 0xf0102ecb <+0>:	endbr32 
   0xf0102ecf <+4>:	push   %ebp
   0xf0102ed0 <+5>:	mov    %esp,%ebp
   0xf0102ed2 <+7>:	sub    $0xc,%esp
   0xf0102ed5 <+10>:	mov    0x8(%ebp),%esp
   0xf0102ed8 <+13>:	popa   
   0xf0102ed9 <+14>:	pop    %es
   0xf0102eda <+15>:	pop    %ds
   0xf0102edb <+16>:	add    $0x8,%esp
   0xf0102ede <+19>:	iret   
   0xf0102edf <+20>:	push   $0xf0105425
   0xf0102ee4 <+25>:	push   $0x1ec
   0xf0102ee9 <+30>:	push   $0xf01053ce
   0xf0102eee <+35>:	call   0xf01000ad <_panic>
End of assembler dump.
(gdb) si 5
=> 0xf0102ed8 <env_pop_tf+13>:	popa   
0xf0102ed8 in env_pop_tf (tf=0x0) at kern/env.c:483
483		asm volatile("\tmovl %0,%%esp\n"
(gdb)
```

### 6. Comprobar, con x/Nx $sp que los contenidos son los mismos que tf (donde N es el tamaño de tf).

```
(gdb) x/17x $sp
0xf01c7000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c7020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c7030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c7040:	0x00000023
```
Efectivamente los contenidos son iguales.

### 7. Describir cada uno de los valores. Para los valores no nulos, se debe indicar dónde se configuró inicialmente el valor, y qué representa.

Los primeros ocho valores pertenecen a los siguientes registros:
```
reg_edi = 0x00000000
reg_esi = 0x00000000
reg_ebp = 0x00000000
reg_oesp = 0x00000000
reg_ebx = 0x00000000
reg_edx = 0x00000000
reg_ecx = 0x00000000
reg_eax = 0x00000000
```

Estos se encuentran en 0 porque todavía el env no comenzo a ejecutarse, por lo tanto, los registros no tienen información

Luego tenemos los valores:

```
tf_es = 0x00000023
tf_ds = 0x00000023
```

Estos valores corresponden a tf_es (extra segment) y tf_ds (data segment), y se inicializan en  `env_alloc()` con los valores `tf_es = GD_UD | 3` y  `tf_ds = GD_UD | 3`. donde `GD_UD = 0x20` es el user data segment y `3` es el ring en que se ejecuta, es decir, user ring

A continuación estan los siguientes valores:

```
tf_trapno = 0x00000000
tf_err =0x00000000
```

tanto tf_trapno como tf_err estan en 0 porque todavia no ocurrio ninguna interrupcion

Finalmente estan los siguientes valores:

```
tf_eip = 0x00800020	
tf_cs = 0x0000001b	
tf_eflags = 0x00000000	
tf_esp = 0xeebfe000
tf_ss = 0x00000023
```

tf_eip apunta a la primera linea del codigo a ejecutar, este valor es cargado por load_icode()

tf_cs se inicializa en  `env_alloc()` con el valor `tf_cs = GD_UT | 3`, donde `GD_UT = 0x18` es el user text segment y `3` es el ring en que se ejecuta, es decir, user ring.

tf_eflags es 0x00000000 debido a que todavia no comenzo la ejecución del env

tf_esp es 0xeebfe000 ya que este es seteado por env_alloc() para apuntar tope del user stack (`USTACKTOP`)

Por ultimo esta tf_ss (stack_segment) que se inicializa en  `env_alloc()` con el valor `tf_ss = GD_UD | 3`. donde `GD_UD = 0x20` es el user data segment y `3` es el ring en que se ejecuta, es decir, user ring.

)

### 8. Continuar hasta la instrucción iret, sin llegar a ejecutarla. Mostrar en este punto, de nuevo, las cinco primeras líneas de info registers en el monitor de QEMU. Explicar los cambios producidos.

Anterior

```
(qemu) info registers
EAX=003bc000 EBX=f01c7000 ECX=f03bc000 EDX=00000214
ESI=00010094 EDI=00000000 EBP=f0118fd8 ESP=f0118fbc
EIP=f0102ecb EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
```



Actual

```
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c7030
EIP=f0102ede EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
```

Como observamos, los registros de propositos general (EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP) cambiaron sus valores.

Tambien se cambio el valor de ES y se cambio su DPL de 0 (kernel) a 3 (user)


### 9. Ejecutar la instrucción iret. En ese momento se ha realizado el cambio de contexto y los símbolos del kernel ya no son válidos.
```
si
=> 0xf0102ede <env_pop_tf+19>:	iret   
0xf0102ede	483		asm volatile("\tmovl %0,%%esp\n"
(gdb) si
=> 0x800020:	cmp    $0xeebfe000,%esp
0x00800020 in ?? ()
```

#### imprimir el valor del contador de programa con p $pc o p $eip
```
(gdb) p $pc
$4 = (void (*)()) 0x800020
```

#### cargar los símbolos de hello con el comando add-symbol-file, así:

```
(gdb) add-symbol-file obj/user/hello 0x800020
add symbol table from file "obj/user/hello" at
	.text_addr = 0x800020
(y or n) y
Reading symbols from obj/user/hello...
```

#### volver a imprimir el valor del contador de programa
```
(gdb) p $pc
$5 = (void (*)()) 0x800020 <_start>
(gdb) 
```

### Mostrar una última vez la salida de info registers en QEMU, y explicar los cambios producidos.

```
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
```

Los cambios producidos son que ahora se pueden observar los valores del Trapframe en los info registers y que el CPL cambio de 0 (kernel) a 3 (user), es decir, hubo un cambio de contexto




### 10. Poner un breakpoint temporal (tbreak, se aplica una sola vez) en la función syscall() y explicar qué ocurre justo tras ejecutar la instrucción int $0x30. Usar, de ser necesario, el monitor de QEMU.


```
EAX=003bc000 EBX=f01c7000 ECX=f03bc000 EDX=00000214
ESI=00010094 EDI=00000000 EBP=f0118fd8 ESP=f0118fbc
EIP=f0102ecb EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
```

Como podemos observar, hubo un cambio de contexto de CPL = 3 (user) a CPL = 0 (kernel). Este comportamiento es el esperado cuando llamamos a una syscall()

...


kern_idt
--------


### Leer user/softint.c y ejecutarlo con make run-softint-nox. ¿Qué interrupción trata de generar? ¿Qué interrupción se genera? Si son diferentes a la que invoca el programa… ¿cuál es el mecanismo por el que ocurrió esto, y por qué motivos? ¿Qué modificarían en JOS para cambiar este comportamiento?


Ejecutamos el programa
```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
TRAP frame at 0xf01c8000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdff0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000d General Protection
  err  0x00000072
  eip  0x00800037
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfd4
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```

Observando el resultado de la ejecución, vemos que se genera la excepción `General Protection`. Esto se debe a que el programa trata de lanzar la excepción 14 que es una Page Fault exception siendo un usario (ring 3) y para lanzar ese tipo de excepciones es necesario ser kernel (ring 0).
Entonces el programa finalaza con una excepción `General Protection`

Para cambiar el comportamiento de JOS, simplemente debemos cambiar de ring 0 a ring 3 cuando establescamos la page fault exception en el idt.

...


user_evilhello
--------------

##### El codigo de evilhello.c original

```
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	// try to print the kernel entry point as a string!  mua ha ha!
	sys_cputs((char*)0xf010000c, 100);
}

```

##### El codigo evilhello.c modificado
```
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    char *entry = (char *) 0xf010000c;
    char first = *entry;
    sys_cputs(&first, 1);
}
```

### ¿En qué se diferencia el código de la versión en evilhello.c mostrada arriba?


La diferencia es que en el original se le pasa directamente a sys_cputs() una direccion de memoria invalida ya que `0xf010000c` pertenece al kernel 

Mientras que en el modificado, se guarda la direccion de memoria invalida en una variable del stack (`first`) y se le pasa esa direccion de la variable a sys_cputs()


### ¿En qué cambia el comportamiento durante la ejecución? ¿Por qué? ¿Cuál es el mecanismo?


##### Ejecucion evilhello.c original

```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
f�rIncoming TRAP frame at 0xefffffbc
[00001000] exiting gracefully
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```


##### Ejecucion evilhello.c modificado

```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
[00001000] user fault va f010000c ip 0080003d
TRAP frame at 0xf01c8000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfd0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000e Page Fault
  cr2  0xf010000c
  err  0x00000005 [user, read, protection]
  eip  0x0080003d
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfb0
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```


Las diferencias en las ejecuciones es que en el original, se logra acceder a la dirección de memoria inválida ya que se llama a sys_cputs() pasandole directamente la dirección de memoria y como esta  no verifica los permisos necesarios, JOS entiende que se hace desde el modo kernel y en consecuecnia se imprime lo que se encuentra en esa direccion: `f�r`.
Es decir, se logra engañar a JOS haciendole creer que nos encontramos en modo kernel

Mientras que en el modificado se intenta acceder a la memoria invalida desde el modo usuario,JOS se da cuenta y en consecuencia se lanza la Page Fault exception

### Listar las direcciones de memoria que se acceden en ambos casos, y en qué ring se realizan. ¿Es esto un problema? ¿Por qué?

En el original: se accede a la memoria `0xf010000c`, pertenenciente al kerenel, desde el ring 0. Esto es un problema ya que el usuario puede acceder a una porción de momoria indebida haciendose pasar por el kernel.

En el modificado: se accede a la memoria `0xf010000c`, pertenenciente al kerenel, desde el ring 3. Sin embargo, en este caso la JOS se da cuenta y lanza una Page Fault exception.




...

