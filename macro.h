#ifndef _INC_MACRO_H_
#define _INC_MACRO_H_

static inline void *smalloc(size_t size)
{
	void *p;
	
	p = malloc(size);
	if (!p) {
		printf("malloc failed!\n");
		exit(1);
	}
	
	return p;
}

static inline void *smalloc_zero(size_t size)
{
	void *p;
	
	p = smalloc(size);
	memset(p, 0, size);
	
	return p;
}

/* Dodatkowe na czas debugowania, smieciowe... */
#define DBP printf("DEBUG POINT (%s), line %d reached\n", __FILE__, __LINE__);
#define DBMPR(t, d) { printf("\n(line=%d) %s\n", __LINE__, (t)); dbm_print((d), dbm_size); printf("\n"); }

#endif /* !_INC_MACRO_H_ */
