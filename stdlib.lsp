(fun {len l}
  {if (== l {})
    {0}
    {+ 1 (len (tail l))}
})
(fun {reverse l} {
  if (== l {})
    {{}}
    {join (reverse (tail l)) (head l)}
})



