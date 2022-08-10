;; NOTE: Assertions have been generated by update_lit_checks.py and should not be edited.
;; RUN: wasm-opt %s --rse --enable-gc-nn-locals -all -S -o - | filecheck %s

(module
 ;; CHECK:      (type $B (struct (field dataref)))

 ;; CHECK:      (type $A (struct (field (ref null data))))
 (type $A (struct_subtype (field (ref null data)) data))

 ;; $B is a subtype of $A, and its field has a more refined type (it is non-
 ;; nullable).
 (type $B (struct_subtype (field (ref data)) $A))

 ;; CHECK:      (func $test
 ;; CHECK-NEXT:  (local $single (ref func))
 ;; CHECK-NEXT:  (local $tuple ((ref any) (ref any)))
 ;; CHECK-NEXT:  (nop)
 ;; CHECK-NEXT: )
 (func $test
  ;; A non-nullable local. The pass should ignore it (as we cannot optimize
  ;; anything here anyhow: the code must assign to the local before reading from
  ;; it, so no sets can be redundant in that sense).
  (local $single (ref func))
  ;; A non-nullable tuple.
  (local $tuple ((ref any) (ref any)))
 )

 ;; CHECK:      (func $needs-refinalize (param $b (ref $B)) (result anyref)
 ;; CHECK-NEXT:  (local $a (ref null $A))
 ;; CHECK-NEXT:  (local.set $a
 ;; CHECK-NEXT:   (local.get $b)
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT:  (struct.get $B 0
 ;; CHECK-NEXT:   (local.get $b)
 ;; CHECK-NEXT:  )
 ;; CHECK-NEXT: )
 (func $needs-refinalize (param $b (ref $B)) (result anyref)
  (local $a (ref null $A))
  ;; Make $a contain $b.
  (local.set $a
    (local.get $b)
  )
  (struct.get $A 0
   ;; Once more, make $a contain $b. This set is redundant. After removing it,
   ;; the struct.get will be reading from type $B, which has a more refined
   ;; field, so we must refinalize to get the right type for the instruction.
   (local.tee $a
    (local.get $b)
   )
  )
 )
)
