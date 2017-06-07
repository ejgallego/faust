/************************************************************************
 ************************************************************************
    FAUST compiler
	Copyright (C) 2003-2018 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    ************************************************************************
    ************************************************************************/

/************************************************************************
 ************************************************************************

    Faust2Wagner --- 2016
    Written by:

    Emilio Jesús Gallego Arias
    Pierre Jouvelot
    Zouhair Mortabit

    ************************************************************************
    ************************************************************************/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <set>
#include <string>

#include "xtended.hh"
#include "binop.hh"
#include "prim2.hh"
#include "wagner_compiler.hh"

#define MEMOMAP_SIZE 1777

const char *binopn[] =
    { "+", "-", "*", "/", "%",
      "<<", ">>",
      ">", "<", ">=", "<=", "==", "!=",
      "&", "|", "^"
    };

// Main compilation routine. We need to be careful to take signal
// sharing into account, Faust internal representation is a graph and
// this is important due to the level of sharing going on due to the
// first-phase macro-unfolding.
//
// [This code was inspired from signals/ppsig.cpp]
wExp* WagnerCompiler::toWagnerExp(Tree sig) {

    int i;
    double r;
    Tree x, y, z, u, le, id, sel, c, label, ff, largs, type, name, file;
    xtended *p = (xtended *) getUserData(sig);

    // Return value
    wExp *ret;

    // Increase count for this expression;
    memo_count[sig]++;

    // Have we processed the current signal? If so, return the cached one.
    for (memo_map* mmap : memo_stack) {

        auto iter = mmap->find(sig);

        if (iter != mmap->end()) {
            // Important as not to create a hash
            return (*iter).second;
        }
    }

    // Else this is a new term.
    if (isList(sig) && len(sig) == 1) {

        // XXX: Is this case necessary?
        ret = toWagnerExp(hd(sig));

    } else if (isList(sig) && len(sig) > 1) {

        // Can we use a vector map?
        std::vector<wExp*> vec;
        Tree tsig = sig;
        do {
            vec.push_back(toWagnerExp(hd(tsig)));
            tsig = tl(tsig);
        } while (isList(tsig));

        ret = new wTuple(vec);

    } else if (isProj(sig, &i, x)) {

        ret = new wProj(toWagnerExp(x), i);

    } else if (isRec(sig, x, le)) {

        // XXX
        std::cerr << "Non-Debruijn expression found!\n";
        ret = new wFeed1(toWagnerExp(le), (string) tree2str (x));

    } else if (isRec(sig, le)) {

        // In debruijn notation, we must open a new scope:
        memo_map *mmap = new memo_map(MEMOMAP_SIZE);
        memo_stack.push_front(mmap);

        wExp *e = toWagnerExp(le);
        memo_stack.pop_front();
        ret = new wFeed(e, mmap);

    } else if (isRef(sig, i)) {

        ret = new wRef(i);

    } else if (getUserData(sig)) {
        // XXX

        int a = sig->arity();
        std::vector<wExp*> vec;

        for (int j = 0; j < a; j++) {
            vec.push_back(toWagnerExp(sig->branch(j)));
        }

        ret = new wFun(p->name (), vec);

    } else if (isSigInt(sig, &i)) {

        ret = new wInteger(i);

    } else if (isSigReal(sig, &r)) {

        ret = new wDouble(r);

    } else if (isSigWaveform(sig)) {

        ret = new wWaveform ("waveform{...}");

    } else if (isSigInput(sig, &i)) {

        ret = new wVar1(i);

    } else if (isSigOutput(sig, &i, x)) {

        ret = new wVar2(i, toWagnerExp(x));

    } else if (isSigDelay1(sig, x)) {

        ret = new wDelay1(toWagnerExp(x));

    } else if (isSigFixDelay(sig, x, y)) {

        ret = new wFixDelay(toWagnerExp(x), toWagnerExp(y));

    } else if (isSigPrefix(sig, x, y)) {

        std::vector<wExp*> vec(2);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        ret = new wFun("prefix", vec);

    } else if (isSigIota (sig, x)) {

        std::vector<wExp*> vec(1);
        vec[0] = toWagnerExp(x);
        ret = new wFun("iota", vec);

    } else if (isSigFFun (sig, ff, largs)) {

        std::vector<wExp*> vec(1);
        vec[0] = toWagnerExp(largs);

        ret = new wFun(ffname(ff), vec);

    } else if (isSigBinOp(sig, &i, x, y)) {

        ret = new wBinaryOp(toWagnerExp(x), toWagnerExp(y), binopn[i]);

    } else if (isSigFConst(sig, type, name, file)) {

        ret = new wWaveform(tree2str(name));

    } else if (isSigFVar(sig, type, name, file)) {

        ret = new wWaveform(tree2str(name));

    } else if (isSigTable(sig, id, x, y)) {

        std::vector<wExp*> vec(2);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        ret = new wFun("table", vec);

    } else if (isSigWRTbl(sig, id, x, y, z)) {

        std::vector<wExp*> vec(3);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        vec[2] = toWagnerExp(z);
        ret = new wFun("write", vec);

    } else if (isSigRDTbl(sig, x, y)) {

        std::vector<wExp*> vec(2);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        ret = new wFun("read", vec);

    } else if (isSigGen(sig, x)) {

        ret = toWagnerExp(x);

    } else if (isSigDocConstantTbl(sig, x, y)) {

        std::vector<wExp*> vec(2);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        ret = new wFun("DocConstantTbl", vec);

    } else if (isSigDocWriteTbl(sig, x, y, z, u)) {

        std::vector<wExp*> vec(4);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        vec[2] = toWagnerExp(z);
        vec[3] = toWagnerExp(u);
        ret = new wFun("DocwriteTbl", vec);

    } else if (isSigDocAccessTbl(sig, x, y)) {

        std::vector<wExp*> vec(2);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        ret = new wFun("DocaccessTbl", vec);

    } else if (isSigSelect2(sig, sel, x, y)) {

        std::vector<wExp*> vec(3);
        vec[0] = toWagnerExp(sel);
        vec[1] = toWagnerExp(x);
        vec[2] = toWagnerExp(y);
        ret = new wFun ("select2", vec);

    } else if (isSigSelect3(sig, sel, x, y, z)) {

        std::vector<wExp*> vec(4);
        vec[0] = toWagnerExp(sel);
        vec[1] = toWagnerExp(x);
        vec[2] = toWagnerExp(y);
        vec[3] = toWagnerExp(z);
        ret = new wFun("select3", vec);

    } else if (isSigIntCast (sig, x)) {

        std::vector<wExp*> vec(1);
        vec[0] = toWagnerExp(x);
        ret = new wFun("int", vec);

    } else if (isSigFloatCast (sig, x)) {

        std::vector<wExp*> vec(1);
        vec[0] = toWagnerExp(x);
        ret = new wFun("float", vec);

    } else if (isSigButton(sig, label)) {

        ret = new wUi("button", (string) tree2str (label));

    } else if (isSigCheckbox(sig, label)) {

        ret = new wUi("checkbox", (string) tree2str (label));

    } else if (isSigVSlider(sig, label, c, x, y, z)) {

        std::vector<wExp*> vec(4);
        vec[0] = toWagnerExp(c);
        vec[1] = toWagnerExp(x);
        vec[2] = toWagnerExp(y);
        vec[3] = toWagnerExp(z);

        ret = new wUi("vslider", (string) tree2str (label), vec);

    } else if (isSigHSlider (sig, label, c, x, y, z)) {

        std::vector<wExp*> vec(4);
        vec[0] = toWagnerExp(c);
        vec[1] = toWagnerExp(x);
        vec[2] = toWagnerExp(y);
        vec[3] = toWagnerExp(z);
        ret = new wUi("hslider", (string) tree2str (label), vec);

    } else if (isSigNumEntry(sig, label, c, x, y, z)) {

        std::vector<wExp*> vec(4);
        vec[0] = toWagnerExp(c);
        vec[1] = toWagnerExp(x);
        vec[2] = toWagnerExp(y);
        vec[3] = toWagnerExp(z);
        ret = new wUi("nentry", (string) tree2str (label), vec);

    } else if (isSigVBargraph(sig, label, x, y, z)) {

        std::vector<wExp*> vec(3);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        vec[2] = toWagnerExp(z);
        ret = new wUi("vbargraph", (string) tree2str (label), vec);

    } else if (isSigHBargraph(sig, label, x, y, z)) {

        std::vector<wExp*> vec(3);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        vec[2] = toWagnerExp(z);
        ret = new wUi("hbargraph", (string) tree2str (label), vec);

    } else if (isSigAttach(sig, x, y)) {

        std::vector<wExp*> vec(2);
        vec[0] = toWagnerExp(x);
        vec[1] = toWagnerExp(y);
        ret = new wFun("attach", vec);
    }

    // Multi rate extensions are disabled until we port the
    // development to Faust 2.0
    /*
      else if (isSigVectorize (sig, x, y))
      {
      std::vector<wExp*> vec(2);
      vec[0] = toWagnerExp(x);
      vec[1] = toWagnerExp(y);
      ret = new wFun ("vectorize", vec);
      }
      else if (isSigSerialize (sig, x))
      {
      std::vector<wExp*> vec(1);
      vec[0] = toWagnerExp(x);
      ret = new wFun ("serialize", vec);
      }
      else if (isSigVectorAt (sig, x, y))
      {
      std::vector<wExp*> vec(2);
      vec[0] = toWagnerExp(x);
      vec[1] = toWagnerExp(y);
      ret = new wFun ("vectorAt", vec);
      }
      else if (isSigConcat (sig, x, y))
      {
      std::vector<wExp*> vec(2);
      vec[0] = toWagnerExp(x);
      vec[1] = toWagnerExp(y);
      ret = new wFun ("concat", vec);
      }
      else if (isSigUpSample (sig, x, y))
      {
      std::vector<wExp*> vec(2);
      vec[0] = toWagnerExp(x);
      vec[1] = toWagnerExp(y);

      ret = new wFun ("up", vec);
      }
      else if (isSigDownSample (sig, x, y))
      {
      std::vector<wExp*> vec(2);
      vec[0] = toWagnerExp(x);
      vec[1] = toWagnerExp(y);

      ret = new wFun ("down", vec);
      }
    */
    else {
        ret = new wError( (string) tree2str(sig));
    }

    // Add the newly computed expression to the memo_map.
    std::pair<Tree, wExp*>res_pair(sig, ret);
    memo_stack.front()->insert(res_pair);

    return new wHash(sig, ret);
}

void WagnerCompiler::compileMultiSignal(Tree L) {

    memo_map *initial_map = new memo_map(MEMOMAP_SIZE);
    memo_stack.push_front(initial_map);

    wExp *wexp = toWagnerExp(L);

    cerr << "Number of elements in the stack: " << memo_stack.size () << endl
         << "************************************************************************"
         << endl;


    // uninline removes let for expressions used only once.
    // collect builds the reachable set
    // We disable both for now, thou uninline could be very useful.

    // cerr << "Number of elements in the hash: " << ht.size() << endl << endl;
    // Pruned tree
    // wexp->uninline(ht);
    // std::set<void*> h_prepruned;
    // wexp->collect(h_prepruned);
    // wexp = wexp->uninline(ht);
    // std::set<void*> h_pruned;
    // wexp->collect(h_pruned);
    // cerr << "Number of elements in the pruned tree: " << h_pruned.size() << endl << endl;

    cerr << "Initial env is: " << endl << endl;
    cerr << initial_map << endl;

    cerr << "************************************************************************"
         << endl;

    cerr << "Program is: " << endl << endl;
    cerr << wexp << endl;

    exit (0);
}


