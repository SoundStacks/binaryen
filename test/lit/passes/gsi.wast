;; NOTE: Assertions have been generated by update_lit_checks.py --all-items and should not be edited.
;; RUN: foreach %s %t wasm-opt --nominal --gsi -all -S -o - | filecheck %s

(module
  ;; CHECK:      (type $struct (struct_subtype (field i32) data))
  (type $struct (struct i32))

  ;; CHECK:      (type $none_=>_none (func_subtype func))

  ;; CHECK:      (global $global1 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  (global $global1 (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (global $global2 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 1337)
  ;; CHECK-NEXT: ))
  (global $global2 (ref $struct) (struct.new $struct
    (i32.const 1337)
  ))

  ;; A non-reference global does not confuse us.
  ;; CHECK:      (global $global-other i32 (i32.const 123456))
  (global $global-other i32 (i32.const 123456))

  ;; CHECK:      (func $test (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (select
  ;; CHECK-NEXT:    (i32.const 42)
  ;; CHECK-NEXT:    (i32.const 1337)
  ;; CHECK-NEXT:    (ref.eq
  ;; CHECK-NEXT:     (ref.as_non_null
  ;; CHECK-NEXT:      (ref.null $struct)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (global.get $global1)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $test
    ;; We can infer that this get can reference either $global1 or $global2,
    ;; and nothing else (aside from a null), and can emit a select between
    ;; those values.
    (drop
      (struct.get $struct 0
        (ref.null $struct)
      )
    )
  )
)

;; Just one global. We do not optimize here - we let other passes do that.
(module
  ;; CHECK:      (type $struct (struct_subtype (field i32) data))
  (type $struct (struct i32))

  ;; CHECK:      (type $none_=>_none (func_subtype func))

  ;; CHECK:      (global $global1 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  (global $global1 (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (func $test (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 0
  ;; CHECK-NEXT:    (ref.null $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $test
    (drop
      (struct.get $struct 0
        (ref.null $struct)
      )
    )
  )
)

;; Three globals. For now, we do not optimize here.
(module
  ;; CHECK:      (type $struct (struct_subtype (field i32) data))
  (type $struct (struct i32))

  ;; CHECK:      (type $none_=>_none (func_subtype func))

  ;; CHECK:      (global $global1 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  (global $global1 (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (global $global2 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 1337)
  ;; CHECK-NEXT: ))
  (global $global2 (ref $struct) (struct.new $struct
    (i32.const 1337)
  ))

  ;; CHECK:      (global $global3 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 99999)
  ;; CHECK-NEXT: ))
  (global $global3 (ref $struct) (struct.new $struct
    (i32.const 99999)
  ))

  ;; CHECK:      (func $test (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 0
  ;; CHECK-NEXT:    (ref.null $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $test
    (drop
      (struct.get $struct 0
        (ref.null $struct)
      )
    )
  )
)

;; A struct.new inside a function stops us from optimizing.
(module
  ;; CHECK:      (type $struct (struct_subtype (field i32) data))
  (type $struct (struct i32))

  ;; CHECK:      (type $none_=>_none (func_subtype func))

  ;; CHECK:      (global $global1 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  (global $global1 (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (global $global2 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 1337)
  ;; CHECK-NEXT: ))
  (global $global2 (ref $struct) (struct.new $struct
    (i32.const 1337)
  ))

  ;; CHECK:      (func $test (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.new $struct
  ;; CHECK-NEXT:    (i32.const 1)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 0
  ;; CHECK-NEXT:    (ref.null $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $test
    (drop
      (struct.new $struct
        (i32.const 1)
      )
    )
    (drop
      (struct.get $struct 0
        (ref.null $struct)
      )
    )
  )
)

;; We ignore imports, as we assume a closed world, but that might change in the
;; future. For now, we will optimize here.
;; TODO: test a reference type here that is imported, and not an i32, but that
;;       is all that validates atm
(module
  ;; CHECK:      (type $struct (struct_subtype (field i32) data))
  (type $struct (struct i32))

  ;; CHECK:      (type $none_=>_none (func_subtype func))

  ;; CHECK:      (import "a" "b" (global $global-import i32))
  (import "a" "b" (global $global-import i32))

  ;; CHECK:      (global $global1 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  (global $global1 (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (global $global2 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 1337)
  ;; CHECK-NEXT: ))
  (global $global2 (ref $struct) (struct.new $struct
    (i32.const 1337)
  ))

  ;; CHECK:      (func $test (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (select
  ;; CHECK-NEXT:    (i32.const 42)
  ;; CHECK-NEXT:    (i32.const 1337)
  ;; CHECK-NEXT:    (ref.eq
  ;; CHECK-NEXT:     (ref.as_non_null
  ;; CHECK-NEXT:      (ref.null $struct)
  ;; CHECK-NEXT:     )
  ;; CHECK-NEXT:     (global.get $global1)
  ;; CHECK-NEXT:    )
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $test
    (drop
      (struct.get $struct 0
        (ref.null $struct)
      )
    )
  )
)

;; A struct.new in a non-toplevel position in a global stops us from
;; optimizing.
(module
  ;; CHECK:      (type $struct (struct_subtype (field i32) data))
  (type $struct (struct i32))

  ;; CHECK:      (type $tuple (struct_subtype (field anyref) (field anyref) data))
  (type $tuple (struct anyref anyref))

  ;; CHECK:      (type $none_=>_none (func_subtype func))

  ;; CHECK:      (global $global1 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 42)
  ;; CHECK-NEXT: ))
  (global $global1 (ref $struct) (struct.new $struct
    (i32.const 42)
  ))

  ;; CHECK:      (global $global2 (ref $struct) (struct.new $struct
  ;; CHECK-NEXT:  (i32.const 1337)
  ;; CHECK-NEXT: ))
  (global $global2 (ref $struct) (struct.new $struct
    (i32.const 1337)
  ))

  ;; CHECK:      (global $global-tuple (ref $tuple) (struct.new $tuple
  ;; CHECK-NEXT:  (struct.new $struct
  ;; CHECK-NEXT:   (i32.const 999999)
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT:  (ref.null any)
  ;; CHECK-NEXT: ))
  (global $global-tuple (ref $tuple) (struct.new $tuple
    (struct.new $struct
      (i32.const 999999)
    )
    (ref.null any)
  ))

  ;; CHECK:      (func $test (type $none_=>_none)
  ;; CHECK-NEXT:  (drop
  ;; CHECK-NEXT:   (struct.get $struct 0
  ;; CHECK-NEXT:    (ref.null $struct)
  ;; CHECK-NEXT:   )
  ;; CHECK-NEXT:  )
  ;; CHECK-NEXT: )
  (func $test
    (drop
      (struct.get $struct 0
        (ref.null $struct)
      )
    )
  )
)
