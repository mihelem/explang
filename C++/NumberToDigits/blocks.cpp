/* © Michele Miccinesi 2019             */                   
/* Test printing integers in bases 2-36	*/
/* Which block size is more efficient?  */
/* TODO: memory alignment               */
/* >= stdc++17							            */

/* EXPLOITING: constexpr, integer_sequence           */
/* ANNOYING: floating operations are not constexpr   */

#include <iostream>
#include <vector>
#include <type_traits>

// The next table has been generated externally with the following code, because
// operations with float cannot be considered compile time! SIGH!
/*
*	#include <iostream>
*	#include <cmath>
*	
*	unsigned int gcd( unsigned int a, unsigned int b ){
*		while( b )
*			std::swap(a%=b, b);
*		return a;
*	}
*	
*	int main(){
*		unsigned int n, d{100}, cd;
*		for( int i=2; i<37; ++i ){
*			d = std::log2(i)*100.;
*			n = 800;
*			cd = gcd(n, d);
*			n /= cd;
*			d /= cd;
*			std::cout << "{"<<n<<","<<d<<"},\n";
*		} 
*	} 	*/
// digits_per_byte, expressed as fractions
constexpr static int digits_per_byte[37][2] { 
	{0,0},
	{0,0},
	{8,1},{400,79},
	{4,1},{100,29},{400,129},{20,7},
	{8,3},{200,79},{200,83},{160,69},{400,179},{80,37},{40,19},{80,39},
	{2,1},{100,51},{25,13},{100,53},{50,27},{800,439},{160,89},{200,113},{400,229},{50,29},{80,47},{32,19},{5,3},{160,97},{80,49},{160,99},
	{8,5},{100,63},{200,127},{25,16},{200,129}
};

struct digit_t{
	char digits[36];
	constexpr digit_t() : digits{}{
		for( char d='0'; d<='9'; ++d )
			digits[d-'0'] = d;
		for( char d='A'; d<='Z'; ++d )
			digits[d-'A'+10] = d;
	}
	constexpr char const& operator[]( int i ) const {
		return digits[i];
	}
};
constexpr const digit_t digits;

constexpr unsigned int pow( const unsigned int& base, unsigned int exp ){ 
	unsigned int r{1};
	while( exp-- )
		r *= base;
	return r;
}

template <	unsigned int digits_per_block, 
			unsigned int base=10, 
			typename = std::enable_if_t<(base<37 && base>1)>
		 >
struct digit_blocks_t{
	constexpr static const int block_number{ pow(base, digits_per_block) };
	using block_type = char[digits_per_block];
	block_type blocks[block_number];

	template <unsigned int position>
	constexpr void filling(unsigned int& i){
		if constexpr( position==digits_per_block )
			++i;
		else{
			int i_digit{0};
			//char d{'0'};
			blocks[i][position] = digits[i_digit];
			filling<position+1>(i);

			while( ++i_digit<base ){
				for( int j=0; j<position; ++j )
					blocks[i][j] = blocks[i-1][j];
				blocks[i][position] = digits[i_digit];
				filling<position+1>(i);
			}
		}
	}

	constexpr digit_blocks_t() : blocks{} {
		unsigned int i{0};
		filling<0>(i);
	}

	constexpr void print() const {
		for( int i=0; i<block_number; ++i ){
			for( int j=0; j<digits_per_block; ++j )
				std::cout << blocks[i][j];
			std::cout << ' ';
		}
	}

	template <typename integer>
	constexpr block_type const & operator[]( integer i ) const {
		return blocks[i];
	}
};

template <unsigned int base, unsigned int block_size>
struct integer_printer_t{
	constexpr static const digit_blocks_t<block_size, base> digit_blocks{};
	static constexpr auto &block_number {digit_blocks.block_number};

	template <typename integer>
	constexpr std::ostream& print( std::ostream& out, const std::vector<integer>& integers ) const {
		constexpr const int digit_count_max{ 1+((1+block_size+sizeof(integer)*digits_per_byte[base][0]/digits_per_byte[base][1])/block_size)*block_size };
		
		char s[digit_count_max];
		s[digit_count_max-1]='\0';

		{
			char sign;
			for( auto n: integers ){
				if( n==0 ){
					out << digits[0] << ' ';
					continue;
				}
				sign = n<0 ? n=-n, '-' : '+';

				int position{digit_count_max-2};
				while( n>0 ){
					integer r{ n % block_number };

					for( const int stop{position-int(block_size)}; position>stop; --position )
						s[position] = digit_blocks[r][position-stop-1];

					n/=block_number;
				}
				while( s[++position]==digits[0] );
				s[--position] = sign;
				
				out << (s+position) << ' ';
			}
		}

		return out;
	}
};

#include <fstream>
#include <iterator>
std::vector<int64_t> i64_from_file( const std::string& filename ){
	std::ifstream file_istream( filename );
	if( !file_istream.is_open() )
		return {};

	std::vector<int64_t> V{ std::istream_iterator<int64_t> (file_istream), std::istream_iterator<int64_t>() };
	return V;
}

#include <chrono>
#include <bitset>

// Testing efficiency of different block_sizes
template <unsigned int base, unsigned int block_size>
void tester( const auto& V, std::ofstream& report ){
	constexpr const integer_printer_t<base, block_size> ip;
	
	auto begin{ std::chrono::high_resolution_clock::now() };
	if constexpr( block_size==0 ){
		for( auto &v: V ){
			if constexpr( base==10 )
				std::cout << v << ' ';	
			else if constexpr( base==2 )
				std::cout << std::bitset<64>(v) << ' ';
		}
	} else
		ip.print( std::cout, V );
	auto end{ std::chrono::high_resolution_clock::now() };
	
	std::cout << '\n';
	report << "BASE: " << base << ", BLOCK_SIZE: " << block_size  << "; " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() << std::endl;
}

#include <utility>	// std::integer_sequence
/* 	The following is to show the use of integer_sequence to implement
*	a compile time for... here I am just showing the explicit use 		
* 	Notice that integer_sequence is often implemented natively for
*	better performance, so it is better than a naive meta-recursion		*/
template < 	unsigned int block_size_min, unsigned int block_size_max, unsigned int block_size_step, 
		   	unsigned int base, 
		   typename = std::enable_if_t< (block_size_min<=block_size_max) >
		 >
struct test_range{
private:
	template <typename integer=int64_t, unsigned int ...Ints>
	constexpr static void helper( const std::vector<integer>& V, std::ofstream& report, std::integer_sequence<unsigned int, Ints...> ){
		(tester<base, block_size_min + Ints*block_size_step>( V, report ), ...);
	}
public:
	test_range() = delete;
	test_range(const test_range&) = delete;
	test_range(test_range&&) = delete;

	template <typename integer=int64_t>
	constexpr static void call( const std::vector<integer>& V, std::ofstream& report ){
		helper( V, report, std::make_integer_sequence<uint, 1+(block_size_max-block_size_min+block_size_step-1)/block_size_step>() );
	}
};

int main(int argc, char *argv[]){
	std::ios_base::sync_with_stdio(false);

	// Example:
	// constexpr digit_blocks_t<3,14> digit_blocks3_14{};
	// digit_blocks3_14.print();
	
	const std::vector<int64_t> V { argc>1 ? i64_from_file(argv[1]) : decltype(V){23948724552,3232485432521,32142142574354398,-2458789213847} };
	std::ofstream report( argc>1 ? std::string(argv[1])+".report" : std::string(argv[0])+".report", std::ios::app );
	if( !report.is_open() )
		std::cout << "Error: cannot create REPORT file" << std::endl;


/* Rather than using the following calls... */
/*#define base 10
	tester<base, 0>( V, report );
	tester<base, 1>( V, report );
	tester<base, 2>( V, report );
	tester<base, 3>( V, report );
	tester<base, 4>( V, report );
	tester<base, 5>( V, report );
#define base 2
	tester<base, 0>( V, report );
	tester<base, 1>( V, report );
	tester<base, 2>( V, report );
	tester<base, 3>( V, report );
	tester<base, 4>( V, report );
	tester<base, 5>( V, report );
	tester<base, 6>( V, report );
	tester<base, 7>( V, report );
	tester<base, 8>( V, report );
	tester<base, 9>( V, report );
	tester<base, 10>( V, report );
	tester<base, 11>( V, report );
	tester<base, 12>( V, report );
	tester<base, 13>( V, report );
	tester<base, 14>( V, report );
	tester<base, 15>( V, report );
	tester<base, 16>( V, report );
#undef base
*/

/* ... just call the next ones... */
	test_range<0, 5, 1, 10>::call( V, report );
	test_range<0, 16, 1, 2>::call( V, report );

	report.flush();	
}
