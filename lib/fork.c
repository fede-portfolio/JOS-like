// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW 0x800


static void
dup_or_share(envid_t dstenv, void *va, int perm)
{
	int r;

	// Si la pagina es de ecritura
	// creo una copia
	if (perm & PTE_W) {
		// Aloco pagina para el proceso hijo
		if ((r = sys_page_alloc(dstenv, va, perm)) < 0)
			panic("dup_or_share: erro on sys_page_alloc: %e", r);

		// Mapeo la pagina del hijo en UTEMP del proceso padre
		if ((r = sys_page_map(dstenv, va, 0, UTEMP, perm)) < 0)
			panic("dup_or_share: error on sys_page_map: %e", r);

		// Copio la pagina del proceso padre al hijo
		memmove(UTEMP, va, PGSIZE);

		// Desmapeo UTEMP del padre ya que era temporal
		if ((r = sys_page_unmap(0, UTEMP)) < 0)
			panic("dup_or_share: error on sys_page_unmap: %e", r);
	} else {
		// Si la pagina es de solo lectura se comparte
		// entre el proceso padre y el hijo
		if ((r = sys_page_map(0, va, dstenv, va, perm)) < 0)
			panic("dup_or_share: error on sys_page_map: %e", r);
	}
}


envid_t
fork_v0(void)
{
	envid_t env_id;
	uint8_t *mem_address;
	int r;

	// Creo un nuevo proceso
	env_id = sys_exofork();
	if (env_id < 0)
		panic("fork_v0: error on sys_fork()");
	if (env_id == 0) {
		// si env_id es 0
		// debemos cambiar el thisenv ya que hace
		// referencia al proceso padre
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// Estoy en el proceso padre
	for (mem_address = 0; mem_address < (uint8_t *) UTOP;
	     mem_address += PGSIZE) {
		pde_t page_dir = uvpd[PDX(mem_address)];

		// Verifico page direcotry esta mapeado
		if (page_dir & PTE_P) {
			pte_t pte = uvpt[PGNUM(mem_address)];

			// Verifico page table entry este mapeada
			if (pte & PTE_P) {
				// La pagina esta mapeada entonces llamo a dup_or_share
				dup_or_share(env_id,
				             (void *) mem_address,
				             pte & PTE_SYSCALL);
			}
		}
	}

	// seteo el proceso hijo como listo para correr
	if ((r = sys_env_set_status(env_id, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return env_id;
}


//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	pte_t pte = uvpt[PGNUM(addr)];

	// Verifico pagina esta mapeada
	if ((err & FEC_PR) == 0) {
		panic("pgfault: Error pagina no mapeada");
	}

	// Verifico pagina esta en modo lectura
	if ((err & FEC_WR) == 0) {
		panic("pgfault: Error pagina en modo lectura");
	}

	// Verifico pagina modo COW
	if ((pte & PTE_COW) == 0) {
		panic("pgfault: Error pagina sin modo COW");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	// Alloco una pagina temporal en PFTEMP
	r = sys_page_alloc(0, PFTEMP, PTE_U | PTE_W | PTE_P);
	if (r < 0) {
		panic("pgfault: Error on sys_page_alloc");
	}

	// Se alinea la direccion de memoria
	addr = ROUNDDOWN(addr, PGSIZE);
	// Copio el contenido de la pagina temporal a addr
	memmove(PFTEMP, addr, PGSIZE);

	// mapeo la pagina temporal en addr
	r = sys_page_map(0, PFTEMP, 0, addr, PTE_W | PTE_U | PTE_P);
	if (r < 0) {
		panic("pgfault: Error on sys_page_map");
	}

	// desmapeo la pagina temporal de PFTEMP
	r = sys_page_unmap(0, PFTEMP);
	if (r < 0) {
		panic("pgfault: Error on sys_page_unmap");
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	pte_t pte = uvpt[pn];
	void *va = (void *) (pn * PGSIZE);

	if (pte & PTE_SHARE) {
		// Si se encuentra el bit PTE_SHARE
		// el hijo comparte los mismos permisos que el padre
		r = sys_page_map(0, va, envid, va, pte & PTE_SYSCALL);
		if (r < 0) {
			panic("duppage: Error on sys_page_map");
		}
	} else if ((pte & PTE_W) || (pte & PTE_COW)) {
		// Si la pagina es de escritura o copy on write
		// Se mapea en el hijo sin PTE_W
		r = sys_page_map(0, va, envid, va, PTE_COW | PTE_U | PTE_P);
		if (r < 0) {
			panic("duppage: Error on sys_page_map");
		}

		// Como se mapeo al hijo con PTE_COW,
		// se debe remapear la página en el padre con los mismos permisos
		r = sys_page_map(0, va, 0, va, PTE_COW | PTE_U | PTE_P);
		if (r < 0) {
			panic("duppage: Error on sys_page_map");
		}
	} else {
		// Si es una pagina de solo lectura se comparte con el padre
		r = sys_page_map(0, va, envid, va, PTE_U | PTE_P);
		if (r < 0) {
			panic("duppage: Error on sys_page_map");
		}
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t env_id;
	uint8_t *mem_address;

	// Seteo el pgfault handler
	set_pgfault_handler(pgfault);

	// Creo el proceso hijo
	env_id = sys_exofork();
	if (env_id < 0)
		panic("fork: error on sys_fork()");
	if (env_id == 0) {
		// si env_id es 0
		// debemos cambiar el thisenv ya que hace
		// referencia al proceso padre
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	// Estoy en el padre
	// itero sobre las paginas de memoria
	for (mem_address = 0; mem_address < (uint8_t *) UTOP;
	     mem_address += PGSIZE) {
		// verifico que no sea la pila de excepciones (UXSTACKTOP - PGSIZE)
		if (mem_address == (uint8_t *) (UXSTACKTOP - PGSIZE)) {
			continue;
		}
		pde_t page_dir = uvpd[PDX(mem_address)];

		// Verifico page direcotry este mapeada
		if (page_dir & PTE_P) {
			pte_t pte = uvpt[PGNUM(mem_address)];

			// Verifico page table entry este mapeada
			if (pte & PTE_P) {
				// La pagina esta mapeada entonces llamo a duppage
				duppage(env_id, PGNUM(mem_address));
			}
		}
	}

	// Alloco pagina para manejo de excepciones del hijo
	int error_page_alloc = sys_page_alloc(env_id,
	                                      (void *) (UXSTACKTOP - PGSIZE),
	                                      PTE_SYSCALL);
	if (error_page_alloc < 0) {
		panic("fork: Error on sys_page_alloc");
	}

	// Seteo el pgfault del hijo, es el manejador de excepciones
	extern void _pgfault_upcall(void);
	int error_pgfault = sys_env_set_pgfault_upcall(env_id, _pgfault_upcall);
	if (error_pgfault < 0) {
		panic("fork: Error on sys_env_set_pgfault_upcall");
	}

	// Seteo el proceso hijo com RUNNABLE
	int error_set_status = sys_env_set_status(env_id, ENV_RUNNABLE);
	if (error_set_status < 0) {
		panic("fork: Error on sys_env_set_status");
	}

	return env_id;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
