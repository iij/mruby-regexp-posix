class MatchData
  attr_reader :length
  attr_reader :regexp
  attr_reader :string

  def [](n)
    if n < @length
      b = self.begin(n)
      e = self.end(n)
      @string[b, e-b]
    else
      nil
    end
  end

  #self[a..b] -> [String]
  #self[a, b] -> [String]

  def captures
    self.to_a[1, length]
  end

  #names : self.regexp.names

  def offset(n)
    [ self.begin(n), self.end(n) ]
  end

  def post_match
    @string[self.end(0), @string.length]
  end

  def pre_match
    @string[0, self.begin(0)]
  end

  def to_a
    a = []
    length.times { |i| a << self[i] }
    a
  end

  def to_s
    self[0]
  end

  def values_at(*idx)
    a = self.to_a
    v = []
    idx.each { |i| v << a[i] }
    v
  end
end

class Regexp
  attr_reader :options
  attr_reader :source

  def self.compile(*args)
    self.new(*args)
  end

  def self.quote
    self.escape
  end

  def casefold?
    (@option & IGNORECASE) > 0
  end

  #def encoding
  #def fixed_encoding?

  # XXX: can't override the operator =~ ?
  def =~(str)
    return nil unless str

    m = self.match(str)
    if m
      m.begin(0)
    else
      nil
    end
  end

  def ===(str)
    unless str.is_a? String
      if str.is_a? Symbol
        str = str.to_s
      else
        return false
      end
    end
    self.match(str) != nil
  end

  def ~
    self =~ $_
  end

  # can be implemented in ruby...?
  #def self.escape
  #end
end
