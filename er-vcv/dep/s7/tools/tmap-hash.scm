;;; hash map timings

(define debugging (provided? 'debugging))

(define chars-upper "#$%&'()*+,-.0123456789:<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{|}¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûü")
(define chars-lower "abcdefghijklmnopqrstuvwxyz-?=") ; more schemish?

(define ok 1000000)
(define bad 10000)

(define (make-strings chr)
  (let* ((num-keys 10000)
	 (keys (make-vector num-keys))
	 (num-chars (length chr)))
    (do ((i 0 (+ i 1)))
	((= i num-keys)
	 keys)
      (let* ((len (+ 4 (random 12)))
	     (str (make-string len)))
	(do ((j 0 (+ j 1)))
	    ((= j len))
	  (string-set! str j (string-ref chr (random num-chars))))
	(vector-set! keys i str)))))


(define (ref-int lim) ; [92, 28 in fx_random_i, 17 in hash_int]
  (let ((H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (int (random 10000) (random 10000)))
	((= i lim))
      (unless (hash-table-ref H int)
	(hash-table-set! H int int)))
    (when debugging (format *stderr* "ref-int: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-int: (6384 10000 0 0 1)

(define (ref-rat lim) ; [4821, 4546 hash_equal_ratio] this is a worst case -- 0..1 mostly and default-hash-table-float-epsilon constrains our options
  (let ((rats (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (/ (+ (random 99) 1) (+ 1 (random 99)))))))
	(H (make-hash-table 1024)))
    ;; slow because ratios need to be comparable with floats: if this were 999 in numerator and 9 in denominator it would be 10 times faster
    (do ((i 0 (+ i 1))
	 (rat (vector-ref rats (random 10000)) (vector-ref rats (random 10000))))
	((= i lim))
      (unless (hash-table-ref H rat)
	(hash-table-set! H rat rat)))
    (when debugging (format *stderr* "ref-rat: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-rat: (16308 36 9 31 2128)

(define (ref-rat1 lim) ; [288, 73 eval]
  (let ((rats (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (/ (+ (random 99999) 1) (+ 1 (random 99)))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (rat (vector-ref rats (random 10000)) (vector-ref rats (random 10000))))
	((= i lim))
      (unless (hash-table-ref H rat)
	(hash-table-set! H rat rat)))
    (when debugging (format *stderr* "ref-rat1: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-rat1: (12516 1849 637 1382 14)

(define (ref-float lim) ; [320, 73 eval, 72 hash_float]
  (let ((floats (let ((V (make-float-vector 10000)))
		  (do ((i 0 (+ i 1)))
		      ((= i 10000) V)
		    (float-vector-set! V i (random 1000.0)))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (float (float-vector-ref floats (random 10000)) (float-vector-ref floats (random 10000))))
	((= i lim))
      (unless (hash-table-ref H float)
	(hash-table-set! H float float)))
    (when debugging (format *stderr* "ref-float: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-float: (15384 2 2 996 21)

(define (ref-complex lim) ; [1133, 945 in hash_float]
  (let ((cs (let ((V (make-vector 10000)))
	      (do ((i 0 (+ i 1)))
		  ((= i 10000) V)
		(vector-set! V i (complex (random 1000.0) (random 1000.0))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (c (vector-ref cs (random 10000)) (vector-ref cs (random 10000))))
	((= i lim))
      (unless (hash-table-ref H c)
	(hash-table-set! H c c)))
    (when debugging (format *stderr* "ref-complex: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-complex: (16374 0 0 10 1065), (14685 199 202 1298 25)

(define (ref-string lim) ; [356 (counting make-strings), 74 eval, 62 for hash_string]
  (let ((H (make-hash-table 1024 string=?))
	(strings (make-strings chars-lower)))
    (do ((i 0 (+ i 1))
	 (str (vector-ref strings (random 10000)) (vector-ref strings (random 10000))))
	((= i lim))
      (unless (hash-table-ref H str)
	(hash-table-set! H str str)))
    (when debugging (format *stderr* "ref-string: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-string: (12795 1412 686 1491 18)

(define (ref-string1 lim) ; [349, 74 eval, 53 hash_string]
  (let ((H (make-hash-table 1024 string=?))
	(strings (make-strings chars-upper)))
    (do ((i 0 (+ i 1))
	 (str (vector-ref strings (random 10000)) (vector-ref strings (random 10000))))
	((= i lim))
      (unless (hash-table-ref H str)
	(hash-table-set! H str str)))
    (when debugging (format *stderr* "ref-string1: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-string1: (9114 5128 1663 479 6)

(define (ref-string2 lim) ; [3915, 3400 hash_string, 80+73 number_to_string]
  (let ((H (make-hash-table 1024 string=?)))
    (do ((i 0 (+ i 1))
	 (str (string-append "w" (number->string (random 10000))) (string-append "w" (number->string (random 10000)))))
	((= i lim))
      (unless (hash-table-ref H str)
	(hash-table-set! H str str)))
    (when debugging (format *stderr* "ref-string2: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-string2: (16374 1 0 9 1111)

(define (ref-string3 lim) ; [344, 73 eval]
  (let* ((syms (symbol-table))
	 (len (length syms))  ; ca 675
	 (strs (make-vector len)))
    (do ((i 0 (+ i 1)))
	((= i len))
      (vector-set! strs i (symbol->string (vector-ref syms i))))
    (let ((H (make-hash-table 1024)))
      (do ((i 0 (+ i 1))
	   (str (vector-ref strs (random len)) (vector-ref strs (random len))))
	  ((= i lim))
	(unless (hash-table-ref H str)
	  (hash-table-set! H str 1)))
      (when debugging (format *stderr* "ref-string3: (~A ~{~A~^ ~})~%"
			      (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most)))))) ; ref-string3: (785 112 47 80 26): (675 786 111 48 79 26)

(define (ref-ci-string lim) ; [856, 586 hash_ci_string]
  (let ((H (make-hash-table 1024 string-ci=?))
	(strings (make-strings chars-lower)))
    (do ((i 0 (+ i 1))
	 (str (vector-ref strings (random 10000)) (vector-ref strings (random 10000))))
	((= i lim))
      (unless (hash-table-ref H str)
	(hash-table-set! H str str)))
    (when debugging (format *stderr* "ref-ci-string: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-ci-string: (16036 0 0 348 42)

(define (ref-sym lim) ; [288, 74 eval]
  (let ((H (make-hash-table 1024))
	(syms (let ((V (make-vector 10000))
		    (strs (make-strings chars-lower)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (string->symbol (vector-ref strs i)))))))
    (do ((i 0 (+ i 1))
	 (sym (vector-ref syms (random 10000)) (vector-ref syms (random 10000))))
	((= i lim))
      (unless (hash-table-ref H sym)
	(hash-table-set! H sym sym)))
    (when debugging (format *stderr* "ref-sym: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-sym: (6412 9945 27 0 2)

(define (ref-sym1 lim) ; [266, 73 eval]
  (let* ((st (symbol-table))
	 (len (length st)))
    (let ((H (make-hash-table 1024)))
      (do ((i 0 (+ i 1))
	   (sym (vector-ref st (random len)) (vector-ref st (random len))))
	  ((= i lim))
	(unless (hash-table-ref H sym)
	  (hash-table-set! H sym 1)))
      (when debugging (format *stderr* "ref-sym1: (~A ~{~A~^ ~})~%"
			      (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most)))))) ; ref-sym1: (493 409 111 11 3)

(define (ref-pair lim) ; [4574, 2936 in pair_equal, 803 integer_equal, 634 hash_equal_any] -> [2495, 1570 pair_equal] -> [659, 172 pair_equal, 76 hash_map_pair]
  (let ((lsts (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (cons (random 1000) (random 1000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref lsts (random 10000)) (vector-ref lsts (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-pair: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most)))))
;; ref-pair: (16284 0 0 100 100): (10000 16185 2 2 195 100): (9953 14600 212 177 1395 21)

(define (ref-pair1 lim) ; [812, 274 hash_map_pair, 150 pair_equal]
  (let ((lsts (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (make-list (random 100) (random 1000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref lsts (random 10000)) (vector-ref lsts (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-pair1: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-pair1: (9404 9039 5561 1529 255 5)

(define (ref-iv lim) ; [442]
  (let ((ivs (let ((V (make-vector 10000)))
	       (do ((i 0 (+ i 1)))
		   ((= i 10000) V)
		 (vector-set! V i (make-int-vector (random 100) (random 1000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref ivs (random 10000)) (vector-ref ivs (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-iv: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-iv: (9407 14335 121 210 1718 12)

(define (ref-bv lim) ; [616]
(let ((bvs (let ((V (make-vector 10000)))
	     (do ((i 0 (+ i 1)))
		 ((= i 10000) V)
	       (vector-set! V i (make-byte-vector (random 100) (random 250))))))
      (H (make-hash-table 1024)))
  (do ((i 0 (+ i 1))
       (p (vector-ref bvs (random 10000)) (vector-ref bvs (random 10000))))
      ((= i lim))
    (unless (hash-table-ref H p)
      (hash-table-set! H p p)))
  (when debugging (format *stderr* "ref-bv: (~A ~{~A~^ ~})~%"
			  (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-bv: (8156 15794 11 8 571 25)

(define (ref-v lim) ; [614]
  (let ((vs (let ((V (make-vector 10000)))
	      (do ((i 0 (+ i 1)))
		  ((= i 10000) V)
		(vector-set! V i (make-vector (random 100) (random 1000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref vs (random 10000)) (vector-ref vs (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-v: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-v: (9433 14334 111 229 1710 13)

(define (ref-fv lim) ; [446]
  (let ((floats (let ((V (make-vector 10000)))
		  (do ((i 0 (+ i 1)))
		      ((= i 10000) V)
		    (vector-set! V i (make-float-vector (random 100) (random 1000.0))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (float (vector-ref floats (random 10000)) (vector-ref floats (random 10000))))
	((= i lim))
      (unless (hash-table-ref H float)
	(hash-table-set! H float float)))
    (when debugging (format *stderr* "ref-fv: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-fv: (9895 14340 113 172 1759 14)

(define (ref-let lim) ; [452, 167 let_equal_1, 65 simple_inlet] -- let_equal checks outlet chains! called from hash_equal_any
  (let ((H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (inlet 'a (random 10000)) (inlet 'a (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-let: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-let: (6384 10000 0 0 1)

(define (ref-let1 lim) ; [1153, 626 let_equal_1]
  (let ((lets (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (inlet 'a (random 1000) 'b (random 1000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref lets (random 10000)) (vector-ref lets (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-let1: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-let1: (14573 208 204 1399 19)

(define (ref-char lim) ; [114, 26 g_random_i, 12 hash_char, 12 integer_to_char]
  (let ((H (make-hash-table 256 char=?)))
    (do ((i 0 (+ i 1))
	 (c (integer->char (random 256)) (integer->char (random 256))))
	((= i lim))
      (unless (hash-table-ref H c)
	(hash-table-set! H c c)))
    (when debugging (format *stderr* "ref-char: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; (768 256 0 0 1)

(define (ref-hash lim) ;[525] slow if hash has > 2 entries
  (let ((tabs (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (hash-table 'a (random 10000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref tabs (random 10000)) (vector-ref tabs (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-hash: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-hash: (16383 0 0 1 6317): (6350 10034 6350 0 0 1)

(define (ref-hash1 lim) ; [555]
  (let ((tabs (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (hash-table 'a (random 10000) 'b (random 10000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref tabs (random 10000)) (vector-ref tabs (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-hash1: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-hash1: (16383 0 0 1 6282): (10000 9135 5105 1657 487 8)

(define (ref-c-pointer lim) ; [352]
  (let ((ptrs (let ((V (make-vector 10000)))
		(do ((i 0 (+ i 1)))
		    ((= i 10000) V)
		  (vector-set! V i (c-pointer (random 4000000))))))
	(H (make-hash-table 1024)))
    (do ((i 0 (+ i 1))
	 (p (vector-ref ptrs (random 10000)) (vector-ref ptrs (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-c-pointer: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-c-pointer: (12794 778 962 1850 10): (9994 9033 5222 1696 433 6)

(define (ref-iterator lim) ; [2882, 887 vector_equal, 705 iterator_equal 216 hash_equal_any]
  (let ((H (make-hash-table 1024)))
    (let ((ptrs (let ((V (make-vector 10000)))
		  (do ((i 0 (+ i 1)))
		      ((= i 10000) V)
		    (vector-set! V i (make-iterator (case (modulo i 6)
						      ((0) (make-list (+ (random 100) 1) (random 10000)))
						      ((1) (vector (random 100) (random 100) (random 100)))
						      ((2) (float-vector (random 100) (random 100) (random 100)))
						      ((3) (int-vector (random 100) (random 100) (random 100)))
						      ((4) (byte-vector (random 100) (random 100) (random 100)))
						      ((5) (string (integer->char (+ (random 50) 32)) 
								   (integer->char (+ (random 50) 32)) 
								   (integer->char (+ (random 50) 32)))))))))))
    (do ((i 0 (+ i 1))
	 (p (vector-ref ptrs (random 10000)) (vector-ref ptrs (random 10000))))
	((= i lim))
      (unless (hash-table-ref H p)
	(hash-table-set! H p p)))
    (when debugging (format *stderr* "ref-iterator: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most)))))) ; ref-iterator: (9982 13546 2146 395 297 87)

(define (ref-undefined lim) ; [25262, 9641 undefined_equal, 9393 strcmp, 5065 hash_equal_any (3 hash_map_undefined)]
                        ; [ 2040, 1666 hash_equal_any, 76 eval (18 hash_map_undefined]
                        ; [  450, 76 eval, 71 hash_equal_any (24 hash_map_undefined]
  (let ((H (make-hash-table 1024))
	(strings (let ((strs (make-strings chars-lower))
		       (V (make-vector 10000)))
		   (do ((i 0 (+ i 1)))
		       ((= i 10000) V)
		     (vector-set! V i (eval-string (string-append "#a" (vector-ref strs i)))))))) ;slightly faster than with-input-from-string + read
    (do ((i 0 (+ i 1))
	 (str (vector-ref strings (random 10000)) (vector-ref strings (random 10000))))
	((= i lim))
      (unless (hash-table-ref H str)
	(hash-table-set! H str str)))
    (when debugging (format *stderr* "ref-undefined: (~A ~{~A~^ ~})~%"
			    (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-undefined: (10000 16372 0 0 12 862): (9999 15542 3 5 834 2443)

(define (ref-c-func lim) ; [2004, 1271 hash_equal_any, 411 eq_equal, 73 eval]
                     ; [340]
  (let* ((st (symbol-table))
	 (len (length st)))
    (let ((H (make-hash-table 1024))
	  (fncs (let ((V (make-vector len #f))
		      (i 0))
		  (for-each (lambda (sym)
			      (let ((f (symbol->value sym)))
				(when (procedure? f)
				  (vector-set! V i f)
				  (set! i (+ i 1)))))
			    st)
		  (set! len i)
		  V)))
      (do ((i 0 (+ i 1))
	   (f (vector-ref fncs (random len)) (vector-ref fncs (random len))))
	  ((= i lim))
	(unless (hash-table-ref H f)
	  (hash-table-set! H f 1)))
      (when debugging (format *stderr* "ref-c-func: (~A ~{~A~^ ~})~%"
			      (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most)))))) ; ref-c-func: (442 632 342 50 0 2)

(when (provided? 'gmp)
  (define (ref-big-int lim) ; [1170]
    (let ((H (make-hash-table 1024)))
      (do ((i 0 (+ i 1))
	   (int (+ 1000000000000000000000000 (random (bignum 10000))) (+ 1000000000000000000000000 (random (bignum 10000)))))
	  ((= i lim))
	(unless (hash-table-ref H int)
	  (hash-table-set! H int int)))
      (when debugging (format *stderr* "ref-big-int: (~A ~{~A~^ ~})~%"
			      (hash-table-entries H) ((object->let H) 'hash-stats:empty|1|2|n|most))))) ; ref-big-int: (10000 6384 10000 0 0 1)
  )

(define (all-cases)
  (ref-int ok)
  (ref-rat bad)
  (ref-rat1 ok)
  (ref-float ok)
  (ref-complex ok)
  (ref-string ok)
  (ref-string1 ok)
  (ref-string2 bad)
  (ref-string3 ok)
  (ref-ci-string ok)
  (ref-sym1 ok)
  (ref-sym ok)
  (ref-pair ok)
  (ref-pair1 ok)
  (ref-iv ok)
  (ref-bv ok)
  (ref-fv ok)
  (ref-v ok)
  (ref-let ok)
  (ref-let1 ok)
  (ref-char ok)
  (ref-hash ok)
  (ref-hash1 ok)
  (ref-c-pointer ok)
  (ref-iterator ok)
  (ref-undefined ok)
  (ref-c-func ok)
  (when (provided? 'gmp)
    (ref-big-int ok))
  )

(all-cases) ; 3.4 secs (27-Mar-26)

(exit)
