/*-----------------------------------------------------------------*/
/* SDCCmem.c - 8051 memory management routines			   */
/*-----------------------------------------------------------------*/

#include "common.h"

/* memory segments */
memmap  *xstack= NULL ;	 /* xternal stack data	       */
memmap  *istack= NULL;	 /* internal stack	           */
memmap  *code  = NULL;	 /* code segment	           */
memmap  *data  = NULL;	 /* internal data upto 128     */
memmap  *xdata = NULL;	 /* external data			   */
memmap  *idata = NULL;	 /* internal data upto 256     */
memmap  *bit   = NULL;	 /* bit addressable space      */
memmap  *statsg= NULL;	 /* the constant data segment  */
memmap  *sfr   = NULL;   /* register space              */
memmap  *reg   = NULL;   /* register space              */
memmap  *sfrbit= NULL;   /* sfr bit space               */
memmap  *generic=NULL;   /* is a generic pointer        */
memmap  *overlay=NULL;   /* overlay segment             */

/* this is a set of sets each set containing
   symbols in a single overlay */
set *ovrSetSets = NULL;    

extern set *operKeyReset ;
extern set *tmpfileSet ;
extern symbol *interrupts[];
int maxRegBank = 0;
int fatalError = 0			 ;/* fatal error flag			*/

/*-----------------------------------------------------------------*/
/* allocMap - allocates a memory map				   */
/*-----------------------------------------------------------------*/
memmap *allocMap (char rspace,     /* sfr space            */
                  char farmap,     /* far or near segment  */
		  char paged ,     /* can this segment be paged  */
		  char direct,     /* directly addressable */
		  char bitaddr,    /* bit addressable space*/
		  char codemap,    /* this is code space   */
		  unsigned sloc,   /* starting location    */
		  const char *name,      /* 2 character name     */
		  char dbName     
		  )
{
	memmap *map ;

	if (!(map = GC_malloc(sizeof(memmap)))) {
		werror(E_OUT_OF_MEM,__FILE__,sizeof(memmap));
		exit (1);
	}

	memset(map, ZERO, sizeof(memmap));
	map->regsp  =  rspace   ;
	map->fmap   =  farmap   ;
	map->paged   =  paged    ;
	map->direct  =  direct   ;
	map->bitsp  =  bitaddr  ;
	map->codesp =  codemap  ;
	map->sloc   =  sloc ;
	map->sname = name ;
	map->dbName = dbName ;
	if (!(map->oFile = tmpfile())) {
		werror(E_TMPFILE_FAILED);
		exit (1);
	} 
	addSetHead (&tmpfileSet,map->oFile);
	map->syms = NULL ;
	return map;
}

/*-----------------------------------------------------------------*/
/* initMem - allocates and initializes all the segments 	   */
/*-----------------------------------------------------------------*/
void initMem ()
{	
	
	/* allocate all the segments */
	/* xternal stack segment ;   
	           SFRSPACE       -   NO
		   FAR-SPACE      -   YES
		   PAGED          -   YES
		   DIRECT-ACCESS  -   NO
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */
	xstack	  = allocMap (0, 1, 1, 0, 0, 0, options.xstack_loc, XSTACK_NAME,'A');

	/* internal stack segment ;   
	           SFRSPACE       -   NO
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   NO
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */
	istack	  = allocMap (0, 0, 0, 0, 0, 0,options.stack_loc, ISTACK_NAME,'B');

	/* code  segment ;   
	           SFRSPACE       -   NO
		   FAR-SPACE      -   YES
		   PAGED          -   NO
		   DIRECT-ACCESS  -   NO
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   YES */
	code 	  = allocMap (0, 1, 0, 0, 0, 1, options.code_loc, CODE_NAME,'C');

	/* Static segment (code for variables );
	           SFRSPACE       -   NO
		   FAR-SPACE      -   YES
		   PAGED          -   NO
		   DIRECT-ACCESS  -   NO
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   YES */
	statsg	  = allocMap (0, 1, 0, 0, 0, 1,0, STATIC_NAME,'D');

	/* Data segment - internal storage segment ;
	           SFRSPACE       -   NO
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   YES
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */	
	data      = allocMap (0, 0, 0, 1, 0, 0, options.data_loc, DATA_NAME,'E');

	/* overlay segment - same as internal storage segment ;
	           SFRSPACE       -   NO
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   YES
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */	
	overlay   = allocMap (0, 0, 0, 1, 0, 0, options.data_loc, DATA_NAME,'E');

	/* Xternal Data segment - 
	           SFRSPACE       -   NO
		   FAR-SPACE      -   YES
		   PAGED          -   NO
		   DIRECT-ACCESS  -   NO
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */
	xdata     = allocMap (0, 1, 0, 0, 0, 0, options.xdata_loc, XDATA_NAME,'F' );

	/* Inderectly addressed internal data segment
	           SFRSPACE       -   NO
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   NO
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */
	idata     = allocMap (0, 0, 0, 0, 0, 0, options.idata_loc,IDATA_NAME,'G' );

	/* Static segment (code for variables );
	           SFRSPACE       -   NO
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   YES
		   BIT-ACCESS     -   YES
		   CODE-ACESS     -   NO */
	bit	  = allocMap (0, 0, 0, 1, 1, 0,0, BIT_NAME,'H');
	
	/* Special function register space :-
	           SFRSPACE       -   YES
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   YES
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */
	sfr        = allocMap (1,0, 0, 1, 0, 0,0, REG_NAME,'I');

	/* Register space ;
	           SFRSPACE       -   YES
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   NO
		   BIT-ACCESS     -   NO
		   CODE-ACESS     -   NO */
	reg        = allocMap (1,0, 0, 0, 0, 0, 0,REG_NAME,' ');

	/* SFR bit space 
	           SFRSPACE       -   YES
		   FAR-SPACE      -   NO
		   PAGED          -   NO
		   DIRECT-ACCESS  -   YES
		   BIT-ACCESS     -   YES
		   CODE-ACESS     -   NO */
	sfrbit     = allocMap (1,0, 0, 1, 1, 0,0, REG_NAME,'J' );

	/* the unknown map */
	generic     = allocMap (1,0, 0, 1, 1, 0,0, REG_NAME,' ' );

}

/*-----------------------------------------------------------------*/
/* allocIntoSeg - puts a symbol into a memory segment              */
/*-----------------------------------------------------------------*/
void allocIntoSeg (symbol *sym) 
{
    memmap *segment = SPEC_OCLS(sym->etype);

    addSet (&segment->syms,sym);
}

/*-----------------------------------------------------------------*/
/* allocGlobal - aassigns the output segment to a global var	   */
/*-----------------------------------------------------------------*/
void allocGlobal ( symbol *sym )
{
    /* symbol name is internal name  */
    sprintf (sym->rname,"_%s",sym->name);
    
    /* add it to the operandKey reset */
    addSet(&operKeyReset,sym);
        
    /* if this is a literal e.g. enumerated type */
    /* put it in the data segment & do nothing   */
    if (IS_LITERAL(sym->etype)) {
	SPEC_OCLS(sym->etype) = data ;
	return ;
    }
       
    /* if this is a function then assign code space    */
    if (IS_FUNC(sym->type)) {
	SPEC_OCLS(sym->etype)  = code ;
	/* if this is an interrupt service routine
	   then put it in the interrupt service array */
	if (IS_ISR(sym->etype)) { 

	    if (interrupts[SPEC_INTN(sym->etype)]) 
		werror(E_INT_DEFINED,
		       SPEC_INTN(sym->etype),
		       interrupts[SPEC_INTN(sym->etype)]->name);
	    else
		interrupts[SPEC_INTN(sym->etype)] = sym;	
	    
	    /* automagically extend the maximum interrupts */
	    if (SPEC_INTN(sym->etype) >= maxInterrupts )
		maxInterrupts = SPEC_INTN(sym->etype) + 1;
	}
	/* if it is not compiler defined */
	if (!sym->cdef)
	    allocIntoSeg(sym);			   

	return ;
    }
        
    /* if this is a  SFR or SBIT */
    if (  SPEC_SCLS(sym->etype) == S_SFR   ||
	  SPEC_SCLS(sym->etype) == S_SBIT  )  {
	
	/* if both absolute address & initial  */
	/* value specified then error	     */
	if ( IS_ABSOLUTE (sym->etype) && sym->ival ) {
	    werror(E_SFR_INIT,sym->name);
	    sym->ival = NULL ;
	}
	
	SPEC_OCLS(sym->etype) = 
	    (SPEC_SCLS(sym->etype) == S_SFR ? sfr : sfrbit);
	
	allocIntoSeg (sym);
	return ;
    }
    
    /* if this is a bit variable and no storage class */
    if ( SPEC_NOUN(sym->etype) == V_BIT	 
	 && SPEC_SCLS(sym->etype) == S_BIT ) {       
	SPEC_OCLS(sym->etype) = bit ;
	allocIntoSeg (sym);
	return  ;
    }

    /* if bit storage class */
    if ( SPEC_SCLS(sym->etype) == S_SBIT ) {
	SPEC_OCLS(sym->etype) = bit;
	allocIntoSeg(sym);
	return ;
    }
    
    /* register storage class ignored changed to FIXED */
    if ( SPEC_SCLS(sym->etype) == S_REGISTER   )
	SPEC_SCLS(sym->etype) = S_FIXED ;
    
    /* if data specified then  */
    if (SPEC_SCLS(sym->etype) == S_DATA)  {
	/* set the output class */
	SPEC_OCLS(sym->etype) = data  ;
	/* generate the symbol  */
	allocIntoSeg (sym) ;
	return   ;
    }
    
    /* if it is fixed, then allocate depending on the  */
    /* current memory model,same for automatics        */
    if ( SPEC_SCLS(sym->etype) == S_FIXED  ||
	 SPEC_SCLS(sym->etype) == S_AUTO   ) {
	/* set the output class */
	SPEC_OCLS(sym->etype) = ( options.model  ? xdata : data ) ;
	/* generate the symbol  */
	allocIntoSeg  (sym) ;
	return   ;
    }
    
    /* if code change to constant */
    if ( SPEC_SCLS(sym->etype) == S_CODE   ||
	 SPEC_SCLS(sym->etype) == S_CONSTANT )  {
	SPEC_OCLS(sym->etype) = statsg   ;
	allocIntoSeg (sym)  ;
	return ;
    }
    
    if ( SPEC_SCLS(sym->etype) == S_XDATA  )    {
	SPEC_OCLS(sym->etype) = xdata  ;
	allocIntoSeg(sym)  ;
	return ;
    }
    
    if ( SPEC_SCLS(sym->etype) == S_IDATA  )    {
	SPEC_OCLS(sym->etype) = idata ;
	sym->iaccess = 1;
	allocIntoSeg (sym)  ;
	return ;
    }
    
    return ;
}

/*-----------------------------------------------------------------*/
/* allocParms - parameters are always passed on stack		   */
/*-----------------------------------------------------------------*/
void allocParms ( value  *val )
{
    value    *lval ;   
    int      pNum = 1;

    for ( lval = val ; lval ; lval = lval->next, pNum++ ) {

	/* check the declaration */
	checkDecl (lval->sym);
	
	//	/* if this is a literal e.g. enumerated type */
	//	if (IS_LITERAL(lval->etype)) {
	//    SPEC_OCLS(lval->etype) = SPEC_OCLS(lval->sym->etype) = 
	//	( options.model  ? xdata : data );
	//    continue;
	//}
	/* if this a register parm then allocate
	   it as a local variable by adding it
	   to the first block we see in the body */
	if (IS_REGPARM(lval->etype)) 
	    continue ;

	/* mark it as my parameter */
	lval->sym->ismyparm = 1;
	lval->sym->localof = currFunc;

	
	/* if automatic variables r 2b stacked */
	if ( options.stackAuto || IS_RENT(currFunc->etype)) {

	    if (lval->sym)
		lval->sym->onStack = 1;

	    /* choose which stack 2 use	  */
	    /*  use xternal stack */
	    if ( options.useXstack )	{
		/* PENDING: stack direction support */
		SPEC_OCLS(lval->etype) = SPEC_OCLS(lval->sym->etype) = xstack ;
		SPEC_STAK(lval->etype) = SPEC_STAK(lval->sym->etype) = lval->sym->stack =
		    xstackPtr - getSize(lval->type);		
		xstackPtr -= getSize (lval->type);
	    }
	    else   {	/* use internal stack   */
		SPEC_OCLS(lval->etype) = SPEC_OCLS(lval->sym->etype) = istack ;
		if (port->stack.direction > 0) {
		    SPEC_STAK(lval->etype) = SPEC_STAK(lval->sym->etype) = lval->sym->stack = 
			stackPtr - ( SPEC_BANK(currFunc->etype) ? port->stack.bank_overhead : 0) - 
			getSize(lval->type) -
			(IS_ISR(currFunc->etype) ? port->stack.isr_overhead : 0); 
		    stackPtr -= getSize (lval->type);
		}
		else {
		    /* This looks like the wrong order but it turns out OK... */
		    /* PENDING: isr, bank overhead, ... */
		    SPEC_STAK(lval->etype) = SPEC_STAK(lval->sym->etype) = lval->sym->stack = stackPtr;
		    stackPtr += getSize (lval->type);
		}		    
	    }
	    allocIntoSeg(lval->sym);
	}
	else   {	/* allocate them in the automatic space */
	    /* generate a unique name  */
	    sprintf (lval->sym->rname,"_%s_PARM_%d",currFunc->name,pNum);
	    strcpy  (lval->name,lval->sym->rname);
	    
	    /* if declared in external storage */
	    if (SPEC_SCLS(lval->etype) == S_XDATA) 
		 SPEC_OCLS(lval->etype) = SPEC_OCLS(lval->sym->etype) = xdata;
	    else
		/* other wise depending on the memory model 
		   note here that we put it into the overlay segment
		   first, we will remove it from the overlay segment
		   after the overlay determination has been done */
		SPEC_OCLS(lval->etype) = SPEC_OCLS(lval->sym->etype) = 
		    ( options.model  ? xdata : (options.noOverlay ? data :overlay ));
	    
	    allocIntoSeg(lval->sym);
	}
    }
    
    return ;
}

/*-----------------------------------------------------------------*/
/* deallocParms - parameters are always passed on stack 	       */
/*-----------------------------------------------------------------*/
void  deallocParms ( value *val )
{
    value    *lval ;
    
    for ( lval = val ; lval ; lval = lval->next ) {

	/* unmark is myparm */
	lval->sym->ismyparm = 0;
	/* if on stack then depending on which stack */
	
	/* delete it from the symbol table  */
	deleteSym (SymbolTab,lval->sym,lval->sym->name); 

	if (!lval->sym->isref) {
	    lval->sym->allocreq = 1;
	    werror(W_NO_REFERENCE,currFunc->name,
		   "function argument",lval->sym->name);
	}

	/* move the rname if any to the name for both val & sym */
	/* and leave a copy of it in the symbol table           */
	if (lval->sym->rname[0]) {
	    char buffer[SDCC_NAME_MAX];
	    strcpy(buffer,lval->sym->rname);
	    lval->sym = copySymbol(lval->sym);
	    strcpy(lval->sym->rname,buffer);
	    strcpy(lval->name,strcpy(lval->sym->name,lval->sym->rname));
	    addSym (SymbolTab, lval->sym, lval->sym->name, 
		    lval->sym->level,lval->sym->block);	    
	    lval->sym->_isparm = 1;
	    addSet(&operKeyReset,lval->sym);
	}

    }
    
    return ;
}

/*-----------------------------------------------------------------*/
/* allocLocal - allocate local variables			   */
/*-----------------------------------------------------------------*/
void allocLocal ( symbol *sym  )
{   
    
    /* generate an unique name */
    sprintf(sym->rname,"_%s_%s_%d_%d",
	    currFunc->name,sym->name,sym->level,sym->block);
    
    sym->islocal = 1;
    sym->localof = currFunc;

    /* if this is a static variable */
    if ( IS_STATIC (sym->etype)) {
	SPEC_OCLS(sym->etype) = (options.model ? xdata : data );
	allocIntoSeg (sym);
        sym->allocreq = 1;
	return   ;
    }
    
    /* if volatile then */
    if (IS_VOLATILE(sym->etype))
	sym->allocreq = 1;

    /* this is automatic	   */

    /* if it to be placed on the stack */
    if ( options.stackAuto || reentrant) {
	       
	sym->onStack = 1;
	if ( options.useXstack ) { 
	    /* PENDING: stack direction for xstack */
	    SPEC_OCLS(sym->etype) = xstack ;
	    SPEC_STAK(sym->etype) = sym->stack = (xstackPtr + 1);
	    xstackPtr += getSize (sym->type) ;
	}
	else {
	    SPEC_OCLS(sym->etype) = istack ;
	    if (port->stack.direction > 0) {
		SPEC_STAK(sym->etype) = sym->stack = ( stackPtr + 1);
		stackPtr += getSize (sym->type) ;
	    }
	    else {
		stackPtr -= getSize (sym->type);
		SPEC_STAK(sym->etype) = sym->stack = stackPtr;
	    }
	}
	allocIntoSeg(sym);
	return ;
    }
    
    /* else depending on the storage class specified */
    if ( SPEC_SCLS(sym->etype) == S_XDATA  )    {
	SPEC_OCLS(sym->etype) = xdata  ;
	allocIntoSeg(sym)  ;
	return ;
    }

    if ( (SPEC_SCLS(sym->etype) == S_CODE   ||
	  SPEC_SCLS(sym->etype) == S_CONSTANT) &&
	 !sym->_isparm)  {
	SPEC_OCLS(sym->etype) = statsg   ;
	allocIntoSeg (sym)  ;
	return ;
    }

    if ( SPEC_SCLS(sym->etype) == S_IDATA  )    {
	SPEC_OCLS(sym->etype) = idata ;
	sym->iaccess = 1;
	allocIntoSeg (sym)  ;
	return ;
    }    

        /* if this is a function then assign code space    */
    if (IS_FUNC(sym->type)) {
	SPEC_OCLS(sym->etype)  = code ;
	return ;
    }
        
    /* if this is a  SFR or SBIT */
    if (  SPEC_SCLS(sym->etype) == S_SFR   ||
	  SPEC_SCLS(sym->etype) == S_SBIT  )  {
	
	/* if both absolute address & initial  */
	/* value specified then error	     */
	if ( IS_ABSOLUTE (sym->etype) && sym->ival ) {
	    werror(E_SFR_INIT,sym->name);
	    sym->ival = NULL ;
	}
	
	SPEC_OCLS(sym->etype) = 
	    (SPEC_SCLS(sym->etype) == S_SFR ? sfr : sfrbit);
	
	allocIntoSeg (sym);
	return ;
    }
    
    /* if this is a bit variable and no storage class */
    if ( SPEC_NOUN(sym->etype) == V_BIT	 
	 && (SPEC_SCLS(sym->etype) == S_BIT)) {       
	SPEC_OCLS(sym->etype) = bit ;
	allocIntoSeg (sym);
	return  ;
    }

    /* again note that we have put it into the overlay segment
       will remove and put into the 'data' segment if required after 
       overlay  analysis has been done */   
    SPEC_OCLS(sym->etype) = ( options.model  ? xdata : 
			      (options.noOverlay ? data : overlay )) ;
    allocIntoSeg (sym); 
}

/*-----------------------------------------------------------------*/
/* deallocLocal - deallocates the local variables                  */
/*-----------------------------------------------------------------*/
void  deallocLocal ( symbol *csym )
{
    symbol *sym ;
    
    for ( sym = csym ; sym ; sym = sym->next) {
	if (sym->_isparm)
	    continue ;

	/* if it is on the stack */
	if (sym->onStack) { 
	    if (options.useXstack)
		xstackPtr -= getSize(sym->type);
	    else
		stackPtr  -= getSize(sym->type);
	}
	/* if not used give a warning */
	if (!sym->isref && !IS_STATIC(sym->etype))
	    werror(W_NO_REFERENCE,currFunc->name,
		   "local variable",sym->name);
	/* now delete it from the symbol table */
	deleteSym (SymbolTab,sym,sym->name);	
    }
}

/*-----------------------------------------------------------------*/
/* overlay2data - moves declarations from the overlay seg to data  */
/*-----------------------------------------------------------------*/
void overlay2data()
{
    symbol *sym;

    for (sym = setFirstItem(overlay->syms); sym;
	 sym = setNextItem(overlay->syms)) {

	SPEC_OCLS(sym->etype) = data;
	allocIntoSeg(sym);
    }

    setToNull((void **) &overlay->syms);
	
}

/*-----------------------------------------------------------------*/
/* overlay2Set - will add all symbols from the overlay segment to  */
/*               the set of sets containing the overlable symbols  */
/*-----------------------------------------------------------------*/
void overlay2Set ()
{
    symbol *sym;
    set *oset = NULL;

    for (sym = setFirstItem(overlay->syms); sym;
	 sym = setNextItem(overlay->syms)) {

	addSet(&oset,sym);
    }
    
    setToNull((void **) &overlay->syms);
    addSet (&ovrSetSets,oset);

}

/*-----------------------------------------------------------------*/
/* allocVariables - creates decl & assign storage class for a v    */
/*-----------------------------------------------------------------*/
int allocVariables ( symbol *symChain )
{
    symbol   *sym;
    symbol   *csym;
    int      stack = 0;    
    int	     saveLevel = 0 ;
    
    /* go thru the symbol chain   */
    for ( sym = symChain ; sym ;  sym = sym->next   ) {
	
	/* if this is a typedef then add it */
	/* to the typedef table             */
	if (IS_TYPEDEF(sym->etype)) {
	    /* check if the typedef already exists    */
	    csym = findSym (TypedefTab, NULL, sym->name );
	    if ( csym && csym->level == sym->level )
		werror(E_DUPLICATE_TYPEDEF,sym->name);
	    
	    addSym (TypedefTab, sym , sym->name,sym->level,sym->block);
	    continue ;  /* go to the next one         */
	}
	/* make sure it already exist */
	csym = findSymWithLevel (SymbolTab, sym);
	if (! csym || (csym && csym->level != sym->level) )
	    csym = sym;
		
	/* check the declaration */
	checkDecl   (csym);
	       	
	/* if this is a function or a pointer to function */
	/* then args  processing  */
	if (funcInChain(csym->type)) {

	    processFuncArgs (csym, 1);
	    /* if register bank specified then update maxRegBank */
	    if (maxRegBank < SPEC_BANK(csym->etype))
		maxRegBank = SPEC_BANK(csym->etype);
	}
	
	/* if this is a extern variable then change the */
	/* level to zero temporarily					*/
	if (IS_EXTERN(csym->etype) || IS_FUNC(csym->type) ) {
	    saveLevel = csym->level ;
	    csym->level = 0 ;
	}
	
	/* if this is a literal then it is an enumerated */
	/* type so need not allocate it space for it     */
	if (IS_LITERAL(sym->etype))
	    continue ;
	
	/* generate the actual declaration  */
	if ( csym->level  ) {
	    allocLocal  (csym);
	    if (csym->onStack)
		stack += getSize(csym->type) ;	    	    
	}
	else 
	    allocGlobal (csym);

	/* restore the level */
	if (IS_EXTERN(csym->etype) || IS_FUNC(csym->type)) 
	    csym->level = saveLevel;		
    }
    
    return stack ;
}

/*-----------------------------------------------------------------*/
/* redoStackOffsets :- will reassign the values for stack offsets  */
/*-----------------------------------------------------------------*/
void redoStackOffsets ()
{
    symbol *sym;
    int sPtr = 0;
    int xsPtr=-1;

    /* after register allocation is complete we know
       which variables will need to be assigned space
       on the stack. We will eliminate those variables
       which do not have the allocReq flag thus reducing
       the stack space */
    for ( sym = setFirstItem(istack->syms); sym;
	  sym = setNextItem(istack->syms)) {
	
	int size = getSize(sym->type);
	/* nothing to do with parameters so continue */
	if ((sym->_isparm && !IS_REGPARM(sym->etype)))
	    continue ;
	
	if ( IS_AGGREGATE(sym->type)) {
	    if (port->stack.direction > 0) {
		SPEC_STAK(sym->etype) = sym->stack = ( sPtr + 1);
		sPtr += size;
	    }
	    else {
		sPtr -= size;
		SPEC_STAK(sym->etype) = sym->stack = sPtr;
	    }
	    continue;
	}

	/* if allocation not required then subtract
	   size from overall stack size & continue */	
	if (!sym->allocreq) {
	    currFunc->stack -= size;
	    SPEC_STAK(currFunc->etype) -= size;
	    continue ;
	}

	if (port->stack.direction > 0) {
	    SPEC_STAK(sym->etype) = sym->stack = ( sPtr + 1);
	    sPtr += size ;
	}
	else {
	    sPtr -= size ;
	    SPEC_STAK(sym->etype) = sym->stack = sPtr;
	}
    }

    /* do the same for the external stack */
    
    for ( sym = setFirstItem(xstack->syms); sym;
	  sym = setNextItem(xstack->syms)) {
	
	int size  = getSize(sym->type);
	/* nothing to do with parameters so continue */
	if ((sym->_isparm && !IS_REGPARM(sym->etype)))
	    continue ;
	
	if (IS_AGGREGATE(sym->type)) {
	    SPEC_STAK(sym->etype) = sym->stack = ( xsPtr + 1);
	    xsPtr += size ;
	    continue ;
	}      

	/* if allocation not required then subtract
	   size from overall stack size & continue */	
	if (!sym->allocreq) {
	    currFunc->xstack -= size;
	    SPEC_STAK(currFunc->etype) -= size;
	    continue ;
	}

	SPEC_STAK(sym->etype) = sym->stack = ( xsPtr + 1);
	xsPtr += size ;
    }

    /* if the debug option is set then output the
       symbols to the map file */
    if (options.debug) {
	for (sym = setFirstItem(istack->syms); sym;
	     sym = setNextItem(istack->syms))
	    cdbSymbol(sym,cdbFile,FALSE,FALSE);

	for (sym = setFirstItem(xstack->syms); sym;
	     sym = setNextItem(xstack->syms))
	    cdbSymbol(sym,cdbFile,FALSE,FALSE);	
    }
}
