(TeX-add-style-hook "First-App"
 (lambda ()
    (LaTeX-add-labels
     "sec:building-simple-user")
    (TeX-run-style-hooks
     "latex2e"
     "art10"
     "article"
     "")))

