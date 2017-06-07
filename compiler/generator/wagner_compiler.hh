////////////////////////////////////////////////////////////////////////
/**
 * Compile a list of FAUST signals into a Wagner Code
 * (c) MINES ParisTech and GRAME 2017
 */
///////////////////////////////////////////////////////////////////////

#ifndef _COMPILE_WAGNER_
#define _COMPILE_WAGNER_

#include "instructions_compiler.hh"
#include "wExp.hh"

class WagnerCompiler : public InstructionsCompiler {

private:

    // Count of used expressions, useful for inlining.
    std::unordered_map<Tree, unsigned int> memo_count;

    // Cache of the translation, following de Bruijn
    std::deque<memo_map*> memo_stack;

    wExp* toWagnerExp(Tree sig);

public:

    WagnerCompiler (CodeContainer* container) :
        InstructionsCompiler(container), memo_count(65543) { };

    virtual void compileMultiSignal (Tree lsig);

};

#endif
