;; Variables for Faust code style in Emacs
((c++-mode . ((tab-width . 4)
              (c-basic-offset . 4)
              ;; XXX Use absolute paths on load of .dir-locals.el
              (faust-architecture-path . "/home/egallego/faust/code/faust-w/architecture/")
              (faust-compiler-path . "/home/egallego/faust/code/faust-w/compiler/")
              (flycheck-disabled-checkers . (c/c++-clang))
              (eval . (setq flycheck-gcc-include-path (append
                                           (mapcar (lambda (x) (concat faust-architecture-path x))
                                                   '(""))
                                           (mapcar (lambda (x) (concat faust-compiler-path x))
                                                   '(""
                                                     "boxes"
                                                     "errors"
                                                     "extended"
                                                     "documentator"
                                                     "evaluate"
                                                     "generator"
                                                     "generator/interpreter"
                                                     "parallelize"
                                                     "parser"
                                                     "signals"
                                                     "tlib")))))
              (flycheck-clang-language-standard . "c++11")
              (flycheck-gcc-language-standard   . "c++11")
              (flycheck-clang-warnings . ("all"))
              (flycheck-gcc-warnings   . ("all"))
              (eval flycheck-mode)
              ;; (flycheck-gcc-include-path '())
              ;; (flycheck-gcc-language-standard "c++11")
              )))

;; http://www.flycheck.org/en/latest/languages.html#c-c
