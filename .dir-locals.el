((nil .
      ((indent-tabs-mode . nil)
       (show-trailing-whitespace . t)))
 (c-mode .
         ((mode . c++)))
 (c++-mode .
          ((eval add-hook 'before-save-hook #'clang-format-buffer nil t))))
