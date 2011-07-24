/** state machine 2 bit **/
// define parameter //
parameter S0 = 3'b000;
parameter S1 = 3'b001;
parameter S2 = 3'b010;
parameter S3 = 3'b011;


// define variable //
reg [1:0] state;


// initialize //
initial begin
	state <= 0;
end


/*
// state machine //
case ( state )
	S0 : begin  // 
		state <= state + 1;
	end

	S1 : begin  // 
		state <= state + 1;
	end

	S2 : begin  // 
		state <= state + 1;
	end

	S3 : begin  // 
		state <= state + 1;
	end

	default : begin  // reset
		state <= S0;
	end
endcase
*/
