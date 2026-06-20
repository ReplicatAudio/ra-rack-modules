;;; implicit ref/set -- tmisc.scm and tread.scm also have some

(set! (*s7* 'heap-size) 1024000)
(load "s7test-block.so" (sublet (curlet) (cons 'init_func 'block_init)))

(define size5 50000)

(define (s1 obj val)
  (set! (obj 'a) val))

(define (s11 obj val)
  (set! ((obj 'b) 'a) val))

(define (s111 obj val)
  (set! (obj 'b 'a) val))

(define (s1111 obj sym val)
  (set! (obj 'b sym) val))

(define (s11111 obj sym val)
  (set! (obj 'b (car sym)) val))


(define (s2 obj val)
  (set! (obj 'a) (+ (log 4 2) val)))

(define (s22 obj val)
  (set! ((obj 'b) 'a) (+ (log 4 2) val)))

(define (s222 obj val)
  (set! (obj 'b 'a) (+ (log 4 2) val)))

(define (s2222 obj sym val)
  (set! (obj 'b sym) (+ (log 4 2) val)))

(define (s22222 obj sym val)
  (set! (obj 'b (car sym)) (+ (log 4 2) val)))


(define (s3 obj val)
  (set! (obj 'b :a) val))


(define (s4 obj val)
  (set! (obj 'b 1) val))

(define (s44 obj val)
  (set! (obj 'b (- (log 4 2) 1)) val))

(define (s444 obj val)
  (set! (obj 'b 1) (car val)))

(define (s4444 obj ind val)
  (set! (obj 'b ind) val))


(define (s5 obj val)
  (set! (obj 0 1) val))

(define (s55 obj val)
  (set! (obj 0 (- (log 4 2) 1)) val))

(define (s555 obj val)
  (set! (obj 0 1) (car val)))

(define (s5555 obj ind val)
  (set! (obj 0 ind) val))


(define (s6 obj val)
  (set! (((obj 'b) 'b) 'a) val))

(define (s66 obj val)
  (set! (((obj 'b) 'b) 'a) val))

(define (s666 obj val)
  (set! (obj 'b 'b 'a) val))

(define (s6666 obj val)
  (set! ((obj 'b) 'b 'a) val))


(define (s7 obj val)
  (set! (list-ref (obj 0) 0) val))

(define (s77 obj i1 i2 val)
  (set! (list-ref (obj i1) i2) val))

(define (s777 obj val)
  (set! ((obj 0)) 32))


(define (s8 obj val)
  (set! (obj 0) val))

(define (s88 obj ind val)
  (set! (obj ind) val))

(define (s888 obj ind val)
  (set! (obj ind) (integer->char val)))

(define (s8888 obj ind val)
  (set! (obj (+ ind 1)) (integer->char val)))

(define (stest n)
  (let ((table (hash-table 'a 1 'b (hash-table 'a 3)))
	(table1 (hash-table 'b "12345"))
	(table2 (vector (vector 1 2 3)))
	(table3 (hash-table 'b (block 1 2 3)))
	;; (table4 (hash-table 'b (let ((x (vector 1 2 3))) (dilambda (lambda (ind) (x ind)) (lambda (ind val) (set! (x ind) val))))))
	(table5 (hash-table 'a 1 'b (hash-table 'a 3 'b (hash-table 'a 4))))
	(table6 (vector (list 0 1) (list 2 3)))
	(envir (inlet 'a 1 'b (inlet 'a 4)))
	(lst (list 0 1))
	(lst1 (list dilambda_test)) ; from s7test-block
	(lst2 (list (list 0 1)))
	(str "0123456789")
	(one 1))

    (do ((i 0 (+ i 1)))
	((= i n))

      (s1 table 12)
      (unless (= (table 'a) 12) (format *stderr* "[1]"))
      (s11 table 12)
      (unless (= ((table 'b) 'a) 12) (format *stderr* "[2]"))
      (s111 table 12)
      (unless (= (table 'b 'a) 12) (format *stderr* "[3]"))
      (s1111 table 'a 12)
      (unless (= (table 'b 'a) 12) (format *stderr* "[4]"))
      (s11111 table '(a) 12)
      (unless (= (table 'b 'a) 12) (format *stderr* "[5]"))

      (s1 envir 12)
      (s11 envir 12)
      (s111 envir 12)
      (s1111 envir 'a 12)
      (s11111 envir '(a) 12)

      (s2 table 12)
      (unless (= (table 'a) 14) (format *stderr* "[6]"))
      (s22 table 12)
      (unless (= ((table 'b) 'a) 14) (format *stderr* "[7]"))
      (s222 table 12)
      (unless (= (table 'b 'a) 14) (format *stderr* "[8]"))
      (s2222 table 'a 12)
      (unless (= (table 'b 'a) 14) (format *stderr* "[9]"))
      (s22222 table '(a) 12)
      (unless (= (table 'b 'a) 14) (format *stderr* "[10]"))

      (s2 envir 12)
      (s22 envir 12)
      (s222 envir 12)
      (s2222 envir 'a 12)
      (s22222 envir '(a) 12)

      (s3 envir 32)
      (unless (= (envir 'b 'a) 32) (format *stderr* "[11]"))

      (s4 table1 #\a) ; set_implicit_string
      (unless (char=? (table1 'b 1) #\a) (format *stderr* "[12]"))
      (s44 table1 #\a)
      (unless (char=? (table1 'b 1) #\a) (format *stderr* "[13]"))
      (s444 table1 '(#\a))
      (unless (char=? (table1 'b 1) #\a) (format *stderr* "[14]"))
      (s4444 table1 1 #\a)
      (unless (char=? (table1 'b 1) #\a) (format *stderr* "[15]"))

      (s4 table3 23.0) ; set_implicit_c_object
      (unless (= (table3 'b 1) 23.0) (format *stderr* "[16]"))
      (s44 table3 23.0)
      (unless (= (table3 'b 1) 23.0) (format *stderr* "[17]"))
      (s444 table3 '(23.0))
      (unless (= (table3 'b 1) 23.0) (format *stderr* "[18]"))
      (s4444 table3 1 23.0)
      (unless (= (table3 'b 1) 23.0) (format *stderr* "[19]"))

;      (s4 table4 23.0) ; set_implicit_closure -- now an error
;      (unless (= (table4 'b 1) 23.0) (format *stderr* "[20]"))
;      (s44 table4 23.0)
;      (unless (= (table4 'b 1) 23.0) (format *stderr* "[21]"))
;      (s444 table4 '(23.0))
;      (unless (= (table4 'b 1) 23.0) (format *stderr* "[22]"))
;      (s4444 table4 1 23.0)
;      (unless (= (table4 'b 1) 23.0) (format *stderr* "[23]"))

      (s5 table2 #\a) ; set_implicit_vector
      (unless (char=? (table2 0 1) #\a) (format *stderr* "[24]"))
      (s55 table2 #\a)
      (unless (char=? (table2 0 1) #\a) (format *stderr* "[25]"))
      (s555 table2 '(#\a))
      (unless (char=? (table2 0 1) #\a) (format *stderr* "[26]"))
      (s5555 table2 1 #\a)
      (unless (char=? (table2 0 1) #\a) (format *stderr* "[27]"))

      (s6 table5 12)
      (unless (= (((table5 'b) 'b) 'a) 12) (format *stderr* "[28]"))
      (s66 table5 12)
      (unless (= ((table5 'b) 'b 'a) 12) (format *stderr* "[29]"))
      (s666 table5 12)
      (unless (= (table5 'b 'b 'a) 12) (format *stderr* "[30]"))
      (s6666 table5 12)
      (unless (= (table5 'b 'b 'a) 12) (format *stderr* "[31]"))

      (s7 table6 12)
      (unless (= (table6 0 0) 12) (format *stderr* "[32]"))
      (s77 table6 0 1 12)
      (unless (= (table6 0 1) 12) (format *stderr* "[33]"))
      (s777 lst1 12)

      (s5 lst2 32)
      (unless (= (cadar lst2) 32) (format *stderr* "[34]"))
      (s55 lst2 12)
      (unless (= (cadar lst2) 12) (format *stderr* "[35]"))
      (s555 lst2 '(15))
      (unless (= (cadar lst2) 15) (format *stderr* "[36]"))
      (s5555 lst2 1 3)
      (unless (= (cadar lst2) 3) (format *stderr* "[37]"))

      (s8 str #\a)
      (unless (char=? (str 0) #\a) (format *stderr* "[38]"))
      (s88 str 1 #\b)
      (unless (char=? (str one) #\b) (format *stderr* "[39]"))
      (s888 str 2 (char->integer #\c))
      (unless (char=? (str (+ one 1)) #\c) (format *stderr* "[40]"))
      (s8888 str 2 (char->integer #\d))
      (unless (char=? (str 3) #\d) (format *stderr* "[41]"))
      (unless (string=? str "abcd456789") (format *stderr* "[42]"))

      )))

(stest size5)


(define len 1000000)

(define H (hash-table 'abs *))
(define (fabsH x)
  ((H 'abs) x 0.0001))

(define (f6 n) ; [719] -> [515 if func_one_arg handles hash] -> [508]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsH i))))))

;(f6 len)


(define P (list + * -))
(define (fabsP x)
  ((P 1) x 0.0001))

(define (f8 n) ; [700] -> [524 fx_implicit]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsP i))))))

;(f8 len)


(define V (vector + * -))
(define (fabsV x)
  ((V 1) x 0.0001))

(define (f9 n) ; [685] -> [512 fx_implicit]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsV i))))))

;(f9 len)


(define C (make-cycle *))
(define (fabsC x)
  ((C) x 0.0001))

(define (f10 n) ; [681] (there is no op_implicit_c_object_ref)
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsC i))))))

;(f10 len)


;;; implicit arg cases (also included elsewhere)
(define B (block .001 .0001 .00001)) ; C-object as arg
(define (fabsB x)
  (* x (B 1)))

(define (f11 n) ; [591] no fx_*_ref?? block_ref_p_pp -> [519] fx_implicit_c_object_ref_a -- why not opt?
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsB i))))))

;(f11 len)


(define P2 (list (list + * -) (list .001 .0001 .00001)))
(define (fabsP2 x)
  ((P2 0 1) x 0.0001))

(define (f12 n) ; [797]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsP2 i))))))

;(f12 len)


(define V2 #2d((#_+ #_* #_-) (.001 .0001 .00001)))
(define (fabsV2 x)
  ((V2 0 1) x 0.0001))

(define (f13 n) ; [778]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsV2 i))))))

;(f13 len)


(define (f14 n) ; [492]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (P2 1 1)))))))

;(f14 len)


(define (f15 n) ; [185] -- [738] if (vector (vector ...))
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (V2 1 1)))))))

;(f15 len)


(define H2 (hash-table 'a .0001))
(define (f16 n) ; [169] -- this is fully optimized!? -> [160] p_pp_sf_href!
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (H2 'a)))))))

;(f16 len)


(define L2 (inlet 'a .0001))
(define (f17 n) ; [173] (no lref) -> [167] lref -> [131 slot_ref]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (L2 'a)))))))

;(f17 len)


(define V3 (vector .0001))
(define (f18 n) ; [148] (opt_p_pi_sc(t_vector_ref_p_pi_unchecked))
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (V3 0)))))))

;(f18 len)


(define P3 (list .0001))
(define (f19 n) ; [157] opt_p_pi_sc(list_ref_p_pi_unchecked)
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (P3 0)))))))

;(f19 len)


(define B3 (block .0001))
(define (f20 n) ; [114] d_7pi_sf(block_ref_d_7pi)
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (B3 0)))))))

;(f20 len)


(define V4 #2d((.0001)))
(define (f21 n) ; [185] opt_p_pii_sff(vector_ref_p_pii_direct)
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (* i (V4 0 0)))))))

;(f21 len)


;;; let cases
(define L (inlet 'abs *))
(define L_abs (L 'abs))

(define (fabs x)
  ((L 'abs) x 0.0001))
  ;((if (integer? x) * /) x 0.0001))

(define (fLabs x)
  (L_abs x 0.0001))

(define (frefabs x)
  ((let-ref L 'abs) x 0.0001))


(define (f1 n) ; [729] -> [507 fx_implicit_let_ref_c]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabs i))))))

;(f1 len)


(define (f2 n) ; [298]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fLabs i))))))

;(f2 len)


(define (f3 n) ; [510]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (frefabs i))))))

;(f3 len)


(define f4 ; [559]
  (let ((L (openlet (inlet '+ (lambda (arg obj)
				(#_+ arg (obj 'value)))
			   'value 3))))
    (lambda (n)
      (do ((i 0 (+ i 1)))
	  ((= i n) (+ 1 L 2))
	(unless (= (+ 1 L 2) 6) ; g_add_3 -> add_p_ppp -> 2 add_p_pp with a method call
	  (display "f4 oops\n"))))))

;(f4 len)


(define (fabsL x)
  ((L 'abs) x 0.0001))

(define (f5 n) ; [512, 723 if set L to H in the loop, 693 if int *??] -> [503?]
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabsL i))))))

;(f5 len)


(define (fabs:L x)
  ((L :abs) x 0.0001))

(define (f22 n) ; [721] -> [504] (added keyword check)
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (fabs:L i))))))

;(f22 len)


(define size 1000000)

(define (f23 n) ; [134] opt'd
  (let ((sum 0.0)
	(L (inlet 'multiply  *)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum ((let-ref L 'multiply) i 0.0001))))))

;(display "f23 ") (display (f23 size)) (newline)


(define (f24 n) ; [605] -> [134 opt'd]
  (let ((sum 0.0)
	(L (inlet 'multiply  *)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum ((L 'multiply) i 0.0001))))))

;(display "f24 ") (display (f24 size)) (newline)


; check if (L 'multiply) is changed opt process is not used (fx):

(define (f24a n) ; [690]
  (let ((sum 0.0)
	(L (inlet 'multiply  *))
	(H (hash-table 'multiply +)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum ((L 'multiply) i 0.0001)))
      (set! L H)))) ; apparently this blocks the optimizer -> op_x_aa etc

;(display "f24a ") (display (f24a size)) (newline)


(define (f24b n) ; [577]
  (let ((sum 0.0)
	(L (inlet 'multiply  *))
	(L1 (inlet 'multiply  +)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum ((L 'multiply) i 0.0001)))
      (set! L L1)))) ; same as above

;(display "f24b ") (display (f24b size)) (newline)


(define (setL L)
  (set! (L 'multiply) floor))

(define (f24c-1 n) ; floor: too many arguments
  (let ((sum 0.0)
	(L (inlet 'multiply  *)))
    (do ((i 0 (+ i 1)))
	((= i 3) sum)
      (set! sum (+ sum ((L 'multiply) i 0.0001)))
      (setL L))))

(define (f24c n) (catch #t (lambda () (f24c-1 n)) (lambda args 'error)))


(define (f24d-1 n) ; same error as above
  (let ((sum 0.0)
	(L (inlet 'multiply  *)))
    (do ((i 0 (+ i 1)))
	((= i 3) sum)
      (set! sum (+ sum ((L 'multiply) i 0.0001)))
      (let-set! L 'multiply floor))))

(define (f24d n) (catch #t (lambda () (f24d-1 n)) (lambda args 'error)))


(define (f25 n) ; [638]
  (let ((sum 0.0)
	(L (inlet 'multiply  *)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum ((L 'multiply) i 0.0001)))
      (let-set! L 'multiply +))))

;(display "f25 ") (display (f25 size)) (newline)

#|
224,053,076  s7.c:eval.isra.0 [/home/bil/motif-snd/repl]  fx_c_scs = 125, op_x_aa = 188
 74,000,000  s7.c:fx_c_scs [/home/bil/motif-snd/repl]     let_set 60
 59,000,225  s7.c:add_p_pp [/home/bil/motif-snd/repl]
 47,999,952  s7.c:fx_implicit_let_ref_c [/home/bil/motif-snd/repl]
 46,999,953  s7.c:op_x_aa [/home/bil/motif-snd/repl]      implicit_let_ref aa="i 0.0001"
 27,000,000  s7.c:let_set_1 [/home/bil/motif-snd/repl]
 24,950,959  s7.c:g_add_x1 [/home/bil/motif-snd/repl]
 21,606,113  s7.c:gc.isra.0 [/home/bil/motif-snd/repl]
 21,000,687  /usr/include/x86_64-linux-gnu/bits/string_fortified.h:eval.isra.0
 19,999,980  s7.c:g_add [/home/bil/motif-snd/repl]
 16,000,016  s7.c:g_num_eq_2 [/home/bil/motif-snd/repl]
 13,000,000  s7.c:let_ref_p_pp [/home/bil/motif-snd/repl]
|#


(require libm.scm)

(define (f26 n) ; if s7 sqrt via *libc* (! sqrt_p_p) [195] sqrt=30 gc=30, via *libm*: [246] sqrt=43 (s7__sqrt)+21 overhead, gc=30
  (let ((sum 0.0))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum ((*libm* 'sqrt) i))))))

;(display "f26 ") (display (f26 size)) (newline)


(define (f27 n) ; [268]
  (let ((sum 0.0)
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (L3 'x)))
      (set! (L3 'x) (* (L3 'x) 0.9999)))))

;(display "f27 ") (display (f27 size)) (newline)   ; 9999.999999994685

;;; (- 3.701520785793392e-44 (expt 0.9999 1000000)) 4.978412222288913e-59

#|
38,942,875  s7.c:opt_dotimes [/home/bil/motif-snd/repl]
30,000,000  s7.c:opt_p_pp_sf_lref [/home/bil/motif-snd/repl]
29,000,000  s7.c:opt_p_ppp_sff [/home/bil/motif-snd/repl]
28,000,144  s7.c:add_p_pp [/home/bil/motif-snd/repl]
26,000,160  s7.c:multiply_p_pp [/home/bil/motif-snd/repl]
26,000,000  s7.c:let_set_1 [/home/bil/motif-snd/repl]            this could be simpler (key check, rootlet/unlet, slot search (save), slot setter in checked_slot_set)
21,689,904  s7.c:gc.isra.0 [/home/bil/motif-snd/repl]
18,000,044  s7.c:let_ref_p_pp [/home/bil/motif-snd/repl]
17,000,000  s7.c:opt_p_pp_fc [/home/bil/motif-snd/repl]
11,000,000  s7.c:opt_p_pp_sf_add [/home/bil/motif-snd/repl]
 9,000,000  s7.c:opt_p_c [/home/bil/motif-snd/repl]              'x in the let-refs, opt_p_pp_sf_lref [build this in], save slot for let-ref too
 9,000,000  s7.c:opt_set_p_p_f [/home/bil/motif-snd/repl]

to:

38,942,785  s7.c:opt_dotimes [/home/bil/motif-snd/repl]
28,000,000  s7.c:add_p_pp [/home/bil/motif-snd/repl]
26,000,470  s7.c:multiply_p_pp [/home/bil/motif-snd/repl]
21,601,157  s7.c:gc.isra.0 [/home/bil/motif-snd/repl]
17,000,000  s7.c:opt_p_pp_fc [/home/bil/motif-snd/repl]
15,000,000  s7.c:opt_p_ppf_slot_set [/home/bil/motif-snd/repl]
11,000,000  s7.c:opt_p_pp_sf_add [/home/bil/motif-snd/repl]
 9,000,000  s7.c:opt_set_p_p_f [/home/bil/motif-snd/repl]
 8,000,000  s7.c:opt_p_pp_slot_ref [/home/bil/motif-snd/repl]
|#

(define (f28 n) ; [86 via opt_p_pp_sc_slot_ref]
  (let ((sum 0.0)
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (let-ref L3 'x))))))

;(display "f28 ") (display (f28 size)) (newline)


(define (f29 n) ; [267 unopt'd except for fx*]
  (let ((sum 0.0)
	(L5 (inlet 'x 0.1))
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1))
	 (L4 L3 L5))
	((= i n) sum)
      (set! sum (+ sum (let-ref L4 'x))))))

;(display "f29 ") (display (f29 size)) (newline)


(define (f30 n) ; [642 eval/inlet_p_pp, gc etc, let_set_1 and let_ref]
  (let ((sum 0.0)
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1))
	 (L4 L3 (inlet 'x 0.1)))
	((= i n) sum)
      (set! (L4 'x) (* 2.0 (L4 'x))) ; make sure it's a new inlet on each iteration
      (set! sum (+ sum (let-ref L4 'x))))))

;(display "f30 ") (display (f30 size)) (newline)


(define (f31 n) ; [105 opt_p_pp_ss_lref let_ref_p_pp] -> [86 opt_p_pp_sc_slot_ref]
  (let ((sum 0.0)
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (let-ref L3 :x))))))

;(display "f31 ") (display (f31 size)) (newline)


(define (f32 n) ; [109 opt_p_pp_sf_lref let_ref_p_pp and opt_p_c for 'x (I assume)] -> [86 slot_ref]
  (let ((sum 0.0)
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (L3 'x))))))

;(display "f32 ") (display (f32 size)) (newline)


(define (f32a n) ; [105] -> [86 slot_ref]
  (let ((sum 0.0)
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (L3 :x))))))

;(display "f32a ") (display (f32a size)) (newline)


(define (f33 n) ; [410, 552 if varlet on every iteration, fx_implicit_let_ref_c]
  (let ((sum 0.0)
	(L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1)))
	((= i n) sum)
      (set! sum (+ sum (L3 'x)))
      (when (= i 0) (varlet L3 'y 0.5)))))

;(display "f33 ") (display (f33 size)) (newline) ; f33: 1000000.0


(define (f34 n) ; [123 let_set_1 from let_set] 64755 (let-set! L3 :x (+ (L3 'x) 1)) -> [82] slot_set and slot_ref
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (let-set! L3 :x (+ (L3 'x) 1)))))

;(display "f34 ") (display (f34)) (newline)

#|
29,000,026  s7.c:let_set_1 [/home/bil/motif-snd/repl]
24,950,959  s7.c:g_add_xi [/home/bil/motif-snd/repl]
19,000,000  s7.c:opt_p_ppp_ssf [/home/bil/motif-snd/repl]            let_set 64750 p_ppp_ok
17,000,000  s7.c:opt_p_pi_fc [/home/bil/motif-snd/repl]
 8,000,008  s7.c:let_set [/home/bil/motif-snd/repl]
 7,000,094  s7.c:opt_dotimes [/home/bil/motif-snd/repl]
 6,924,479  s7.c:gc.isra.0 [/home/bil/motif-snd/repl]
 4,000,000  s7.c:opt_p_pp_sc_slot_ref [/home/bil/motif-snd/repl]     let-refs
 3,000,000  s7.c:add_p_pi [/home/bil/motif-snd/repl]

  to:

24,950,959  s7.c:g_add_xi [/home/bil/motif-snd/repl]
17,000,000  s7.c:opt_p_pi_fc [/home/bil/motif-snd/repl]
15,000,000  s7.c:opt_p_ppf_slot_set [/home/bil/motif-snd/repl]
 7,000,094  s7.c:opt_dotimes [/home/bil/motif-snd/repl]
 6,924,494  s7.c:gc.isra.0 [/home/bil/motif-snd/repl]
 4,000,000  s7.c:opt_p_pp_slot_ref [/home/bil/motif-snd/repl]
 3,000,000  s7.c:add_p_pi [/home/bil/motif-snd/repl]
|#


(define (f34a n) ; [133] opt_p_ppp_sff -> let_set(_1) 64800 (let-set! L3 'x (+ (L3 'x) 1)) -> [82] as above
  (let ((L3 (inlet 'x 1.0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (let-set! L3 'x (+ (L3 'x) 1)))))

;(display "f34a ") (display (f34a size)) (newline)


(define (f35 n) ; [141] -> [98]
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (set! (L3 'x) (+ (L3 'x) 1)))))

;(display "f35 ") (display (f35 size)) (newline)


(define (f35a n) ; [33] opt_p_pps_slot_set
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (set! (L3 :x) i))))

;(display "f35a ") (display (f35a size)) (newline)


(define (f35b n) ; [66] -> [32] opt_p_ppc_slot_set -- twice as slow as f35i due to make_integer in opt_dotimes (set! is op_safe_do, let-set! is op_safe_dotimes in check_do)
                 ;      -> [14] if sequence allows op_safe_dotimes (set! (L :x) val) as opposed to (let-set! L :x val)
                 ;         but set! lacks has_fn and op_simple_do (the fallback) expects has_fn, so we lose
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (set! (L3 :x) 0))))

;(display "f35b ") (display (f35b size)) (newline)


(define (f35c n) ; [98] opt_p_ppf_slot_set and opt_p_pp_slot_ref
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (set! (L3 :x) (+ (L3 'x) 1)))))

;(display "f35c ") (display (f35c size)) (newline)


(define (f35d n) ; [61] 65890 -> [33] opt_p_pps_slot_set
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (set! (L3 'x) i))))

;(display "f35d ") (display (f35d size)) (newline)


(define (f35e n) ; [45] ppf_slot_set -> [32] opt_p_ppc_slot_set
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (set! (L3 'x) 0))))

;(display "f35e ") (display (f35e size)) (newline) ; 0


(define (f35f n) ; [63] -> [35] opt_p_pps_slot_set
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (let-set! L3 'x i))))

;(display "f35f ") (display (f35f size)) (newline) ; 999999


(define (f35g n) ; [29] opt_p_ppf_slot_set + opt_p_c -> [16] opt_p_ppc_slot_set
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (let-set! L3 'x 0))))

;(display "f35g ") (display (f35g size)) (newline) ; 0


(define (f35h n) ; [72] 64738 -> [35] opt_p_pps_slot_set
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (let-set! L3 :x i))))

;(display "f35h ") (display (f35h size)) (newline) ; 999999


(define (f35i n) ; [58] opt_p_ppp_ssc + let_set_1 -> [16] opt_ppc_slot_set
  (let ((L3 (inlet 'x 0)))
    (do ((i 0 (+ i 1)))
	((= i n) (L3 :x))
      (let-set! L3 :x 0))))

;(display "f35i ") (display (f35i size)) (newline)


(define (test-all)
  (unless (= (f1 len) 4.999994999999997e+7) (format *stderr* "f1 ~A\n" (f1 len)))
  (unless (= (f2 len) 4.999994999999997e+7) (format *stderr* "f2 ~A\n" (f2 len)))
  (unless (= (f3 len) 4.999994999999997e+7) (format *stderr* "f3 ~A\n" (f3 len)))
  (unless (= (f4 len) 6) (format *stderr* "f4 ~A\n" (f4 len)))
  (unless (= (f5 len) 4.999994999999997e+7) (format *stderr* "f5 ~A\n" (f5 len)))
  (unless (= (f6 len) 4.999994999999997e+7) (format *stderr* "f6 ~A\n" (f6 len)))
  (unless (= (f8 len) 4.999994999999997e+7) (format *stderr* "f8 ~A\n" (f8 len)))
  (unless (= (f9 len) 4.999994999999997e+7) (format *stderr* "f9 ~A\n" (f9 len)))
  (unless (= (f10 len) 4.999994999999997e+7) (format *stderr* "f10 ~A\n" (f10 len)))
  (unless (= (f11 len) 4.999994999999997e+7) (format *stderr* "f11 ~A\n" (f11 len)))
  (unless (= (f12 len) 4.999994999999997e+7) (format *stderr* "f12 ~A\n" (f12 len)))
  (unless (= (f13 len) 4.999994999999997e+7) (format *stderr* "f13 ~A\n" (f13 len)))
  (unless (= (f14 len) 4.999994999999997e+7) (format *stderr* "f14 ~A\n" (f14 len)))
  (unless (= (f15 len) 4.999994999999997e+7) (format *stderr* "f15 ~A\n" (f15 len)))
  (unless (= (f16 len) 4.999994999999997e+7) (format *stderr* "f16 ~A\n" (f16 len)))
  (unless (= (f17 len) 4.999994999999997e+7) (format *stderr* "f17 ~A\n" (f17 len)))
  (unless (= (f18 len) 4.999994999999997e+7) (format *stderr* "f18 ~A\n" (f18 len)))
  (unless (= (f19 len) 4.999994999999997e+7) (format *stderr* "f19 ~A\n" (f19 len)))
  (unless (= (f20 len) 4.999994999999997e+7) (format *stderr* "f20 ~A\n" (f20 len)))
  (unless (= (f21 len) 4.999994999999997e+7) (format *stderr* "f21 ~A\n" (f21 len)))
  (unless (= (f22 len) 4.999994999999997e+7) (format *stderr* "f22 ~A\n" (f22 len)))

  (unless (equal? (f23 size) 4.999994999999997e+7) (display "f23: ") (display (f23 size)) (newline))
  (unless (equal? (f24 size) 4.999994999999997e+7) (display "f24: ") (display (f24 size)) (newline))
  (unless (equal? (f24a size) 499999500104.7288) (display "f24a: ") (display (f24a size)) (newline))
  (unless (equal? (f24b size) 499999500104.7288) (display "f24b: ") (display (f24b size)) (newline))
  (unless (equal? (f24c size) 'error) (display "f24c: ") (display (f24c size)) (newline))
  (unless (equal? (f24d size) 'error) (display "f24d: ") (display (f24d size)) (newline))
  (unless (equal? (f25 size) 499999500104.7288) (display "f25: ") (display (f25 size)) (newline))
  (unless (equal? (f26 size) 6.666661664588418e+8) (display "f26: ") (display (f26 size)) (newline))
  (unless (equal? (f27 size) 9999.999999994685) (display "f27: ") (display (f27 size)) (newline))
  (unless (equal? (f28 size) 1000000.0) (display "f28: ") (display (f28 size)) (newline))
  (unless (equal? (f29 size) 1.0000090000133294e+5) (display "f29: ") (display (f29 size)) (newline))
  (unless (equal? (f30 size) 2.0000180000266587e+5) (display "f30: ") (display (f30 size)) (newline))
  (unless (equal? (f31 size) 1000000.0) (display "f31: ") (display (f31 size)) (newline))
  (unless (equal? (f32 size) 1000000.0) (display "f32: ") (display (f32 size)) (newline))
  (unless (equal? (f32a size) 1000000.0) (display "f32a: ") (display (f32a size)) (newline))
  (unless (equal? (f33 size) 1000000.0) (display "f33: ") (display (f33 size)) (newline))
  (unless (equal? (f34 size) 1000000) (display "f34: ") (display (f34 size)) (newline))
  (unless (equal? (f34a size) 1000001.0) (display "f34a: ") (display (f34a size)) (newline))
  (unless (equal? (f35 size) 1000000) (display "f35: ") (display (f35 size)) (newline))
  (unless (equal? (f35a size) 999999) (display "f35a: ") (display (f35a size)) (newline))
  (unless (equal? (f35b size) 0) (display "f35b: ") (display (f35b size)) (newline))
  (unless (equal? (f35c size) 1000000) (display "f35c: ") (display (f35c size)) (newline))
  (unless (equal? (f35d size) 999999) (display "f35d: ") (display (f35d size)) (newline))
  (unless (equal? (f35e size) 0) (display "f35e: ") (display (f35e size)) (newline))
  (unless (equal? (f35f size) 999999) (display "f35f: ") (display (f35f size)) (newline))
  (unless (equal? (f35g size) 0) (display "f35g: ") (display (f35g size)) (newline))
  (unless (equal? (f35h size) 999999) (display "f35h: ") (display (f35h size)) (newline))
  (unless (equal? (f35i size) 0) (display "f35i: ") (display (f35i size)) (newline))
)

(test-all)

(exit)
