/************************************************************************
 ************************************************************************
    FAUST compiler
	Copyright (C) 2003-2004 GRAME, Centre National de Creation Musicale

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

////////////////////////////////////////////////////////////////////////
// Abstract class for Wagner expressions                              //
// Copyright (C) 2017 GRAME -- MINES ParisTech                        //
////////////////////////////////////////////////////////////////////////

#ifndef _WEXP_
#define _WEXP_

#include <functional>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <set>
#include <tuple>
#include <iostream>
#include "tree.hh"

class wExp {

public:
    wExp () { }

    virtual void collect(std::set<void*>& set) = 0;
    virtual wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) = 0;
    virtual ostream& print (ostream& fout) const = 0;

};

inline ostream& operator << (ostream& file, const wExp& e) { return e.print(file);  }
inline ostream& operator << (ostream& file, const wExp *e) { return e->print(file); }

typedef std::unordered_map<Tree, wExp*> memo_map;

inline ostream& operator << (ostream& fout, const memo_map *mmap) {

    std::map<Tree, wExp*> temp_mmap;

    // Order the hashes
    for (std::pair<Tree,wExp*> expr : *mmap) {
        temp_mmap.insert(expr);
	}

	for (std::pair<Tree,wExp*> expr : temp_mmap) {
		cerr << "let [" << expr.first << "]" << " = " << expr.second << " in" << endl;
	}

    return fout;
}

////////////////////////////////////////////////////////////////////////
// Reference to an expression using a hash table.
////////////////////////////////////////////////////////////////////////
class wHash : public wExp {

public:

    Tree tref;
    wExp *eref;

    wHash(Tree tref, wExp *eref) : tref(tref), eref(eref) { }

    void collect(std::set<void*>& set) {

        // cerr << "marking: " << ref << endl;
        set.insert(tref);
        eref->collect(set);

    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {

        eref->uninline(cmap);

        if(cmap[tref] == 1) {
            // cerr << "uninlining " << tref << " | " << eref << endl;
            return eref;
        } else {
            return this;
        }
    }

    ostream& print(ostream& out) const {
        return out << "P[" << tref << "]";
    }

};

////////////////////////////////////////////////////////////////////////
// Integer and double constants                                       //
////////////////////////////////////////////////////////////////////////
class wInteger : public wExp {

public:

    int value;

    wInteger (int val):value (val) { }

    void collect(std::set<void*>& set) {
        return;
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        return this;
    }

    ostream & print (ostream& out) const {
        out << value;
        return out;
    }
};

class wDouble : public wExp {

public:

    double value;

    wDouble(double val) : value (val) { }

    void collect(std::set<void*>& set) {
        return;
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        return this;
    }

    ostream& print(ostream& out) const {
        return out << value;
    }

};

////////////////////////////////////////////////////////////////////////
// Variables (input and output)                                       //
////////////////////////////////////////////////////////////////////////
class wVar1 : public wExp {

public:

    // This choice implies that variable shifting must create a copy of
    // the constructor.
    int idx;

    wVar1(int v_idx) : idx(v_idx) { }

    void collect(std::set<void*>& set) {
        return;
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        return this;
    }

    ostream& print(ostream& out) const {
        return out << "IN_" << idx;
    }

};

class wVar2 : public wExp {

public:

    int idx;
    wExp *x;

    wVar2(int v_idx, wExp * e) : idx(v_idx), x(e) { }

    void collect(std::set<void*>& set) {
        return;
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        return this;
    }

    ostream& print (ostream& out) const {
        return out << "OUT" << idx << x;
    }
};

//this class for FConst, FVar and waveforme signals
class wWaveform : public wExp {

public:

    string str;

    wWaveform (string mess):str (mess) { }

    void collect(std::set<void*>& set) {
        return;
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        return this;
    }

    ostream& print (ostream& out) const
    {
        return out << str;
    }
};

////////////////////////////////////////////////////////////////////////
// Binary operators                                                   //
////////////////////////////////////////////////////////////////////////
class wBinaryOp : public wExp {

public:

    std::tuple<wExp*,wExp*> args;
    const char *op;

    wBinaryOp(wExp *e1, wExp *e2, const char *c) : args(e1, e2), op(c) { }

    //wBinaryOp(wExp *e1, wExp *e2) : args(e1, e2) { }
    //wBinaryOp(std::tuple<wExp*, wExp*> l) : args(l) { }

    void collect(std::set<void*>& set) {
        std::get<0>(args)->collect(set);
        std::get<1>(args)->collect(set);
    }


    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        std::get<0>(args) = std::get<0>(args)->uninline(cmap);
        std::get<1>(args) = std::get<1>(args)->uninline(cmap);

        return this;
    }

    ostream & print (ostream& out) const {
        return out << "(" << std::get<0>(args) << " " << op
                   << " " << std::get<1>(args) <<
            ")";
    }

};

////////////////////////////////////////////////////////////////////////
// Projections                                                        //
////////////////////////////////////////////////////////////////////////
class wProj : public wExp {

public:

    wExp *p_exp;
    int p_idx;

    wProj(wExp * exp, int idx) : p_exp(exp), p_idx(idx) { }

    void collect(std::set<void*>& set) {
        p_exp->collect(set);
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {

        p_exp = p_exp->uninline(cmap);
        return this;

    }

    ostream & print (ostream& out) const {
        return out << "(" << p_exp << ")" << "." << p_idx;
    }
};

////////////////////////////////////////////////////////////////////////
//                                                                    //
////////////////////////////////////////////////////////////////////////
class wDelay1 : public wExp {

public:

    wExp* d_exp;

    wDelay1(wExp * exp) : d_exp(exp) { }

    void collect(std::set<void*>& set) {
        d_exp->collect(set);
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        d_exp = d_exp->uninline(cmap);
        return this;
    }

    ostream & print (ostream& out) const {
        return out << "mem(" << d_exp << ")";
    }

};

class wFixDelay : public wExp {

public:

    std::tuple<wExp*, wExp*> args;

    wFixDelay(wExp *e1, wExp *e2) : args(e1, e2) { }

    void collect(std::set<void*>& set) {
        std::get<0>(args)->collect(set);
        std::get<1>(args)->collect(set);
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {

        std::get<0>(args) = std::get<0>(args)->uninline(cmap);
        std::get<1>(args) = std::get<1>(args)->uninline(cmap);

        return this;
    }

    ostream & print (ostream& out) const {
        return out << std::get<0>(args) << "@" << std::get<1>(args);
    }

};
////////////////////////////////////////////////////////////////////////
// Feedback                                                           //
// We assume deBruijn notation, but that could be changed!            //
////////////////////////////////////////////////////////////////////////
class wFeed : public wExp {

public:
    //wFeed(std::initializer_list<wExp*> l) : eqns(l) { }

    // List of local expressions in the context of the feedback + actual feed.
    wExp* exp;
    memo_map *mmap;

    wFeed(wExp* ex, memo_map *mm) : exp(ex), mmap(mm) { }

    void collect(std::set<void*>& set) {
        exp->collect(set);
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        exp = exp->uninline(cmap);
        return this;
    }

    ostream & print(ostream& out) const {
        return out << "Feed = \n" << mmap << "\n" << exp << endl;
    }

};

class wFeed1 : public wExp {

public:

    wExp* exp;
    string x;

    wFeed1(wExp * ex, string s):exp (ex), x (s) { }

    void collect(std::set<void*>& set) {
        exp->collect(set);
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        exp = exp->uninline(cmap);
        return this;
    }

    ostream& print(ostream& out) const {
        return out << "Fees1 =" << x << exp;
    }

};

class wRef : public wExp {

public:

    int ref;

    wRef(int val) : ref(val) { }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        return this;
    }

    void collect(std::set<void*>& set) {
        return;
    }

    ostream& print (ostream& out) const
    {
        return out << "REF[" << ref << "]";
    }

};

////////////////////////////////////////////////////////////////////////
// Tuples                                                             //
////////////////////////////////////////////////////////////////////////
class wTuple : public wExp {

public:

    std::vector<wExp*> elems;

    wTuple(std::initializer_list<wExp*> l) : elems(l) { }
    wTuple(std::vector<wExp*>           l) : elems(l) { }

    void collect(std::set<void*>& set) {
        for(auto& e : elems) { e->collect(set); }
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        for(auto& e : elems) { e->uninline(cmap); }
        return this;
    }

    ostream & print (ostream& out) const {
        out << "(";
        for(auto& e : elems) out << e << ",";
        out << ")";

        return out;
    }

};

////////////////////////////////////////////////////////////////////////
// Function and Interface                                            //
////////////////////////////////////////////////////////////////////////
class wFun : public wExp {

public:

    string f_name;
    std::vector<wExp*> elems;

    wFun(string s, std::vector < wExp * >l) : f_name(s), elems(l) { }

    void collect(std::set<void*>& set) {
        for(auto& e : elems) {
            e->collect(set);
        }
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {

        for(auto& e : elems) {
            e->uninline(cmap);
        }

        return this;
    }

    ostream& print (ostream& out) const {
        out << f_name << "(";

        // Must move to an infix_iterator
        for(auto& e : elems) {
            out << e << (e != elems.back() ? "," : "");
        }

        return out << ")";
    }
};

class wUi : public wExp {

public:

    string name;
    string label;
    std::vector < wExp * >elems;

    wUi (string s, string lb, std::vector < wExp * >l):name (s), label (lb), elems (l) { }
    wUi (string s, string lb):name (s), label (lb) { }

    void collect(std::set<void*>& set) {
        for(auto& e : elems) {
            e->collect(set);
        }
    }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {

        for(auto& e : elems) {
            e->uninline(cmap);
        }

        return this;
    }

    ostream& print (ostream& out) const {
        return out << name;
    }

};

class wError : public wExp {

public:

    //Tree sig;
    string s;

    wError (string err) : s (err) {  }

    void collect(std::set<void*>& set) { }

    wExp *uninline(std::unordered_map<Tree, unsigned int>& cmap) {
        return this;
    }

    ostream& print (ostream & out) const {
        return out << "ERROR[" << s << "]";
    }

};

#endif
