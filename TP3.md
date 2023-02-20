TP3: Multitarea con desalojo
============================

sys_yield
---------


Leer y estudiar el código del programa user/yield.c. Cambiar la función i386_init() para lanzar tres instancias de dicho programa, y mostrar y explicar la salida de make qemu-nox.

Código de yield.c

```
// yield the processor to other environments

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int i;

	cprintf("Hello, I am environment %08x, cpu %d.\n", thisenv->env_id, thisenv->env_cpunum);
	for (i = 0; i < 5; i++) {
		sys_yield();
		cprintf("Back in environment %08x, iteration %d, cpu %d.\n",
			thisenv->env_id, i, thisenv->env_cpunum);
	}
	cprintf("All done in environment %08x.\n", thisenv->env_id);
}

```

Salida de QEMU

```
MP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000, cpu 0.
Hello, I am environment 00001001, cpu 0.
Hello, I am environment 00001002, cpu 0.
Back in environment 00001000, iteration 0, cpu 0.
Back in environment 00001001, iteration 0, cpu 0.
Back in environment 00001002, iteration 0, cpu 0.
Back in environment 00001000, iteration 1, cpu 0.
Back in environment 00001001, iteration 1, cpu 0.
Back in environment 00001002, iteration 1, cpu 0.
Back in environment 00001000, iteration 2, cpu 0.
Back in environment 00001001, iteration 2, cpu 0.
Back in environment 00001002, iteration 2, cpu 0.
Back in environment 00001000, iteration 3, cpu 0.
Back in environment 00001001, iteration 3, cpu 0.
Back in environment 00001002, iteration 3, cpu 0.
Back in environment 00001000, iteration 4, cpu 0.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4, cpu 0.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001002, iteration 4, cpu 0.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
Welcome to the JOS kernel monitor!
```

Como se puede ver en la salida de qemu,se crean 3 procesos sobre los que se iteran 5 veces. 
En cada iteración los procesos le ceden el control al scheduler y son desalojados. 
Como el scheduler es Round-Robin le asigna a todos los procesos mismo tiempo de CPU. Por eso cuando termina de ejecutar un proceso busca el siguiente proceso y lo ejecuta.
Esta particularidad se ve cuando termina de ejecutar el proceso 00001000, se ejecuta el siguiente 00001001, y cuando este termina se ejecuta 00001002. Como es una lista circular y teniamos 3 procesos cuando termina 00001002 se vuelve a ejecutar 00001000. Asi hasta que terminan de ejecutar todos los procesos.

dumbfork
--------
Si una página no es modificable en el padre ¿lo es en el hijo? En otras palabras: ¿se preserva, en el hijo, el flag de solo-lectura en las páginas copiadas?

En el hijo NO se preserva el flag de solo lectura, ya que se setean los siguientes flags:
```
sys_page_alloc(dstenv, addr, PTE_P|PTE_U|PTE_W)
```

Donde PTE_W (permiso de escritura) hace a la página escribible (writable).


Mostrar, con código en espacio de usuario, cómo podría dumbfork() verificar si una dirección en el padre es de solo lectura, de tal manera que pudiera pasar como tercer parámetro a duppage() un booleano llamado readonly que indicase si la página es modificable o no:

```
envid_t dumbfork(void) {
    // ...
    for (addr = UTEXT; addr < end; addr += PGSIZE) {
        bool readonly = true;
        if ((uvpd[PDX(addr)] & PTE_P){
            if((uvpt[PGNUM(addr)] & PTE_P)){
                if (uvpt[PGNUM(addr)] & PTE_W){
                    readonly = false;
                }
            }
        }
        duppage(envid, addr, readonly);	
    }
}
```
Supongamos que se desea actualizar el código de duppage() para tener en cuenta el argumento readonly: si este es verdadero, la página copiada no debe ser modificable en el hijo. Es fácil hacerlo realizando una última llamada a sys_page_map() para eliminar el flag PTE_W en el hijo, cuando corresponda:
Se pide mostrar una versión en el que se implemente la misma funcionalidad readonly, pero sin usar en ningún caso más de tres llamadas al sistema.

```
void duppage(envid_t dstenv, void *addr, bool readonly) {
    if(readonly == true){
        permisos = PTE_P | PTE_U ;
    }else{
        perm = PTE_P | PTE_U | PTE_W;
    }
    sys_page_alloc(dstenv, addr, permisos);
    sys_page_map(dstenv, addr, 0, UTEMP, permisos);

    memmove(UTEMP, addr, PGSIZE);
    sys_page_unmap(0, UTEMP);
}


ipc_recv
--------
Un proceso podría intentar enviar el valor númerico -E_INVAL vía ipc_send(). ¿Cómo es posible distinguir si es un error, o no?

Version catedra
```
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (/* ??? */)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")

```


Version resuelta
```
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (src == 0)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")

```

Verifico si src es 0, ya que, `ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)` pone en 0 *from_env_store si hubo un error.

sys_ipc_try_send
----------------

Se pide ahora explicar cómo se podría implementar una función sys_ipc_send() (con los mismos parámetros que sys_ipc_try_send()) que sea bloqueante, es decir, que si un proceso A la usa para enviar un mensaje a B, pero B no está esperando un mensaje, el proceso A sea puesto en estado ENV_NOT_RUNNABLE, y despertado una vez B llame a ipc_recv() (cuya firma no debe ser cambiada).

Se podria hacer algo similar a lo que hace sys_ipc_recv, es decir, tener una variable booleana  que indique si el proceso se encuentra bloqueado mandando un mensaje. 
Además, se tendria que guardar en una cola o array de cada proceso receptor los env_id de cada proceso que le intenta mandar un mensaje y por último, setear a los procesos que se bloquean mandando un mensaje como ENV_NOT_RUNNABLE.

qué cambios se necesitan en struct Env para la implementación (campos nuevos, y su tipo; campos cambiados, o eliminados, si los hay)

agregar una vairalbe booleana que indique si el proceso esta bloqueado enviando un mensaje.

Una cola o un array de los procesos que le estan intentando mandar un mensaje



qué asignaciones de campos se harían en sys_ipc_send()

Se setearia a la variable bool env_ipc_sending

Se setearia al proceso como ENV_NOT_RUNNABLE.

Se agregaria a la cola o array del receptor el ID del proceso que desea enviar.




qué código se añadiría en sys_ipc_recv()

Se agregaria el código vinculado a popear de la cola o el array a los procesos que intentan mandar un mensaje, para indicarles que el destinatario se encuentra recibiendo mensajes.



¿existe posibilidad de deadlock?

Si la hay, por eso debemos asgurarnos que se envie un mensaje a un proceso que no este bloqueado enviando, ya que esto generaria que dos procesos se queden bloqueados enviando


¿funciona que varios procesos (A₁, A₂, …) puedan enviar a B, y quedar cada uno bloqueado mientras B no consuma su mensaje? ¿en qué orden despertarían?

Si, lo hace gracias a que se guardan los procesos que envian en una cola o un array.

En caso de utilizar una cola los procesos que envian se despertarian siguiendo la política FIFO, es decir, el primero que llega es el primero que se levanta.

En caso de utilizarse un array se podria utilizar la misma politica que la cola, o realizarla a la inversa, es decir, se despierta el ultimo en llegar. Sin embargo esto no es recomendable ya que si se recben muchos mensajes es posibles que los primeros procesos nunca sean despertados.

